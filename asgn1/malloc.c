#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define META_SIZE sizeof(struct block_meta)
#define MALLOC_ALIGNMENT 16
#define MIN_BLOCK_SIZE 16
#define SBRK_INCR 64000
#define align16(x) (((((x)-1) >> 4) << 4) + 16)
#define align4(x) (((((x)-1) >> 2) << 2) + 4)

// this macro VA_ARGS trick requires gcc
#define debug_print(fmt_str, ...)                                              \
  do {                                                                         \
    if (getenv("DEBUG_MALLOC")) {                                              \
      const char* mc_fmt = fmt_str;                                            \
      int mc_sz = snprintf(NULL, 0, mc_fmt, __VA_ARGS__);                      \
      char mc_buf[mc_sz + 1];                                                  \
      snprintf(mc_buf, sizeof(mc_buf), mc_fmt, __VA_ARGS__);                   \
      setvbuf(stderr, (char*)NULL, _IONBF, 0);                                 \
      fputs(mc_buf, stderr);                                                   \
    }                                                                          \
  } while (0)

typedef struct block_meta* block_meta_t;

struct block_meta {
  size_t size;
  struct block_meta* next;
  struct block_meta* prev;
  bool free;
};

void* global_base = NULL;

/* When we get a request of some size, we iterate through our linked list to
 * see if there’s a free block that’s large enough. The returned block may
 * be far too big for what we need, this function only guarantees finding one
 * at least as large as /size/
 * RETURNS: The function returns a fitting chunk, or NULL if none were found.
 * After the execution, the argument last points to the last visited chunk
 */
block_meta_t find_free_block(block_meta_t* last, size_t size) {
  debug_print("Finding free block with last=%p and size=%zu\n", last, size);
  block_meta_t current = global_base;
  while (current && !current->free && current->size < size) {
    *last = current;
    current = current->next;
  }
  return current;
}

// split a block into two pieces, with the first being the provided size and
// the second being what remains
void split_block(block_meta_t block_to_split, size_t size) {
  block_meta_t new;
  new = (block_meta_t)(((char*)(block_to_split + META_SIZE)) + size);
  new->size = block_to_split->size - size - META_SIZE;
  new->next = block_to_split->next;
  new->prev = block_to_split;
  new->free = true;
  block_to_split->size = size;
  block_to_split->next = new;
  if (new->next) {
    new->next->prev = new;
  }
}

size_t round_up(size_t numToRound, uint16_t multiple) {
  if (multiple == 0)
    return numToRound;

  uint32_t remainder = numToRound % multiple;
  if (remainder == 0)
    return numToRound;

  return numToRound + multiple - remainder;
}

void* sbrk_round_up(size_t size) {
  void* old = sbrk(0);
  void* request = sbrk(round_up(size, SBRK_INCR));

  if ((int)request < 0) {
    errno = ENOMEM;
    debug_print("request_space: failed to move break of size %zu, no memory\n",
                size);
    return NULL; // sbrk failed.
  }
  debug_print("memory break successfully moved from %p to %p\n", old, sbrk(0));
  return request;
}

/* Ask for a block of space (extend heap). Request space from the OS using
 * sbrk and add our new block to the end of the linked list.
 * TODO: Don't sbrk for every single request for space
 */
block_meta_t request_space(block_meta_t last, size_t size) {
  block_meta_t b;
  // save current break since we aren't totally sure where it is
  void* old_brk = sbrk(0);
  if (last) {
    // not our first request for space
    // find address at the very end of the last malloc cell
    void* end_of_list_ptr = ((void*)(last + 1) + last->size);

    if ((old_brk - end_of_list_ptr) < (META_SIZE + size)) {
      debug_print("no room at end of heap, extending\n", NULL);
      // no space for new cell between last cell and break, break needs to move
      // try to bump the brk up by the next multiple of SBRK_INCR
      if (!sbrk_round_up(META_SIZE + size)) {
        // sbrk failed to increase
        return NULL;
      }
    } else {
      debug_print("space request fits under current break\n", NULL);
    }
    b = (block_meta_t)end_of_list_ptr;
  } else {
    // first request for space, sbrk always has to move
    debug_print("first request for space, moving sbrk\n", NULL);
    if (!sbrk_round_up(META_SIZE + size)) {
      return NULL;
    }
    b = old_brk;
  }

  // last will be NULL on first request for space.
  if (last) {
    last->next = b;
  }
  b->size = size;
  b->next = NULL;
  b->prev = last;
  b->free = false;
  return b;
}

// attempt to fuse with the next block, if it exists and is free.
// if not, returns the block given to it unmodified
block_meta_t fuse_with_next(block_meta_t block) {
  if (block->next && block->next->free) {
    block->size += META_SIZE + block->next->size;
    block->next = block->next->next;
    if (block->next) {
      // if there was a block after the successor, point it's previous block
      // to our current (now merged) block
      block->next->prev = block;
    }
  } else {
    if (!block->next) {
      debug_print("fuse failed, no next block, returning original\n", NULL);
    } else {
      debug_print("fuse failed, next block not free, returning original\n",
                  NULL);
    }
  }
  return block;
}

// return whether the pointer is within the block's size range
bool ptr_within_block(block_meta_t block, void* ptr) {
  return ptr >= (void*)(block) && ptr < (void*)(block + 1) + block->size;
}

void* malloc(size_t size) {
  block_meta_t block;

  if (size <= 0) {
    return NULL;
  }

  size_t s = align16(size);

  if (!global_base) {
    // first malloc call
    block = request_space(NULL, s);
    if (!block) {
      return NULL;
    }
    global_base = block;
  } else {
    // we have previously allocated memory
    struct block_meta* last = global_base;
    // search for a free block, also updates last to the final block in the
    // heap in case we need to extend because no free block found
    block = find_free_block(&last, s);
    if (!block) {
      // no free blocks found
      debug_print("no free blocks found\n", NULL);
      block = request_space(last, s);
      if (!block) {
        return NULL;
      }
    } else {
      // free block found
      if ((block->size - s) >= META_SIZE + MIN_BLOCK_SIZE) {
        // split the block because there is room for another in the top end
        split_block(block, s);
      }
      block->free = false;
    }
  }
  // return the address right ahead of the block struct
  // (keep in mind this is pointer arithmetic)
  void* p = block + 1;
  debug_print("MALLOC: malloc(%d)     =>  (ptr=%p, size=%d)\n", size, p,
              block->size);
  return p;
}

void* calloc(size_t num_elems, size_t elem_size) {
  size_t size = num_elems * elem_size; // TODO: check for overflow
  void* ptr = malloc(size);
  debug_print("MALLOC: calloc(%d, %d)  =>  (ptr=%p, size=%d)\n", num_elems,
              elem_size, ptr, size);
  return memset(ptr, 0, size);
}

// takes a pointer to memory returned to by an *alloc function and walks
// block list in order to find which block the ptr is within.
block_meta_t get_ptr_block(void* ptr) {
  block_meta_t b = global_base;
  while (b) {
    if (ptr_within_block(b, ptr)) {
      return b;
    } else {
      b = b->next;
    }
  }
  // no block found that contains pointer (this shouldn't happen)
  debug_print("NO BLOCK FOUND containing pointer (SHOULD NOT HAPPEN)\n", NULL);
  return NULL;
}

// either returns the block of the valid pointer OR NULL if the address
// is invalid for any reason
block_meta_t valid_addr(void* p) {
  debug_print("checking if pointer %p is valid \n", p);
  if (global_base) {
    // malloc has been called at least once
    void* heap_top = sbrk(0);
    debug_print("heap_base => %p and heap_top => %p\n", global_base, heap_top);
    if (p >= global_base && p <= heap_top) {
      // pointer is within the heap address range, see if we can find it's block
      debug_print("pointer in valid address range, getting block\n", NULL);
      return get_ptr_block(p);
    }
    debug_print("pointer not within heap range\n", NULL);
  } else {
    debug_print("no global base, has malloc been called?\n", NULL);
  }
  // address was invalid
  return NULL;
}

// TODO: WHY DOES MALLOC + FREE 1MIL times fail
void free(void* ptr) {

  debug_print("MALLOC: free(%p)\n", ptr);
  if (!ptr) {
    return;
  }

  block_meta_t b = valid_addr(ptr);
  if (b) {
    debug_print("FREEING valid ptr\n", NULL);
    // ptr was a valid address
    b->free = true;
    // attempt to fuse with previous if possible
    // NOTE: we call fuse_with_next STARTING with the previous block
    if (b->prev && b->prev->free) {
      debug_print("PREVIOUS block detected, attempting fuse\n", NULL);
      b = fuse_with_next(b->prev);
    }
    // now see if there is a free block after us and attempt to fuse
    if (b->next) {
      debug_print("NEXT block detected, attempting fuse\n", NULL);
      fuse_with_next(b);
    } else {
      debug_print("end block in heap list freeing\n", NULL);
      // we are the last block in the heap, not necessarily the only one though
      if (b->prev) {
        // there is a block before us, clear it's ref to us
        b->prev->next = NULL;

      } else {
        // we are the only block in heap
        #if (__APPLE__ && __MACH__)
        // leave a block in the heap for mac, since we can't move the break
        // downwards. If we try to set global_base to NULL it will always move
        // the break on every malloc after all frees have happened
        debug_print("all blocks free, leaving one b/c MacOS\n", NULL);
        #else
        // sbrk works properly, move it downwards
        // global_base = NULL;
        // debug_print("Only block freed, heap list empty\n", NULL);
        // brk(b);
        // debug_print("moving brk to %p\n", sbrk(0));
        #endif
      }
    }
  } else {
    debug_print("invalid ptr, didn't free\n", NULL);
  }
}

void copy_block(block_meta_t src, block_meta_t dst) {
  int *sdata, *ddata;
  size_t i;
  sdata = (int*)(src + 1);
  ddata = (int*)(dst + 1);
  for (i = 0; i * 4 < src->size && i * 4 < dst->size; i++) {
    ddata[i] = sdata[i];
  }
}

// TODO: WHY IS REALLOC MOVING BUFFER WHEN IT SHOULDNT
void* realloc(void* ptr, size_t size) {
  if (!ptr) {
    // NULL ptr, realloc should act like malloc
    return malloc(size);
  }

  if (size <= 0) {
    free(ptr);
    return NULL;
  }
  block_meta_t b = valid_addr(ptr);
  if (b) {
    debug_print("pointer valid, proceeding with realloc\n", NULL);

    size_t s = align16(size);
    if (b->size >= s) {
      // requested size is equal or smaller than current size
      if (b->size - s >= (META_SIZE + MIN_BLOCK_SIZE)) {
        // if requested smaller size leaves enough room for another block,
        // split our block
        split_block(b, s);
      }

    } else {
      // requested size larger than current block, try to fuse to get the
      // required space if possible
      if (b->next && b->next->free &&
          ((b->size + META_SIZE + b->next->size) >= s)) {
        // next block is free and provides big enough space,
        // so fuse them together
        fuse_with_next(b);
        if (b->size - s >= (META_SIZE + MIN_BLOCK_SIZE)) {
          // if the new fused larger block has enough extra space for a new
          // empty block, do a split
          split_block(b, s);
        }

      } else {
        // next block can't be used because either isn't free or too small,
        // or it doesn't exist because we are at the end of the heap.
        // thus we need to move our current block to the end of the heap in a
        // new correctly sized block
        void* new_ptr = malloc(s);
        if (!new_ptr) {
          // malloc failed, you're doomed
          errno = ENOMEM;
          return NULL;
        }
        block_meta_t new = get_ptr_block(new_ptr);
        copy_block(b, new);


        debug_print("MALLOC: realloc(%p, %d) =>  (ptr=%p, size=%d)\n", ptr,
                    size, new_ptr, s);
        free(ptr);
        return new_ptr;
      }
    }

    // here we succeeded fusing with next block or had enough space in
    // original block, so we just give back the original ptr
    return ptr;

  } else {
    // address invalid, can't realloc
    debug_print("Pointer invalid or block not found", NULL);
    return NULL;
  }
}
