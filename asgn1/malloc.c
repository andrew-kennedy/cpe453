#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define META_SIZE sizeof(struct block_meta)
#define MALLOC_ALIGNMENT 16
#define MIN_BLOCK_SIZE 16
#define align16(x) (((((x)-1) >> 4) << 4) + 16)
#define align4(x) (((((x)-1) >> 2) << 2) + 4)

#ifdef DEBUG_MALLOC
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define debug_print(fmt_str, ...) \
  do { \
    if (DEBUG_TEST) { \
      const char *fmt = fmt_str; \
      int sz = snprintf(NULL, 0, fmt, __VA_ARGS__); \
      char buf[sz + 1]; \
      snprintf(buf, sizeof buf, fmt, __VA_ARGS__); \
      fputs(buf, stderr); \
    } \
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
 */
block_meta_t find_free_block(block_meta_t* last, size_t size) {
  block_meta_t current = global_base;
  while (current && !(current->free && current->size >= size)) {
    *last = current;
    current = current->next;
  }
  return current;
}

// split a block into two pieces, with the first being the provided size and
// the second being what remains
void split_block(block_meta_t block_to_split, size_t size) {
  block_meta_t new;
  new = (block_meta_t)((char*)(block_to_split + META_SIZE)) + size;
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

/* Ask for a block of space (extend heap). Request space from the OS using
 * sbrk and add our new block to the end of the linked list.
 * TODO: Don't sbrk for every single request for space
 */
block_meta_t request_space(block_meta_t last, size_t size) {
  block_meta_t b;
  // save current break since we aren't totally sure where it is
  b = sbrk(0);
  void* request = sbrk(size + META_SIZE);
  if ((int)request < 0) {
    errno = ENOMEM;
    return NULL; // sbrk failed.
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

size_t round_up(size_t numToRound, uint8_t multiple) {
  if (multiple == 0)
    return numToRound;

  uint32_t remainder = numToRound % multiple;
  if (remainder == 0)
    return numToRound;

  return numToRound + multiple - remainder;
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
  }
  return block;
}

bool ptr_within_block(block_meta_t block, void* ptr) {
  return ptr >= (void*)(block + 1) && ptr < (void*)block->next;
}

void* malloc(size_t size) {
  block_meta_t block;

  if (size <= 0) {
    return NULL;
  }

  size = align16(size);

  if (!global_base) {
    // first malloc call
    block = request_space(NULL, size);
    if (!block) {
      return NULL;
    }
    global_base = block;
  } else {
    // we have previously allocated memory
    struct block_meta* last = global_base;
    block = find_free_block(&last, size);
    if (!block) {
      // no free blocks found
      block = request_space(last, size);
      if (!block) {
        return NULL;
      }
    } else {
      // free block found
      if ((block->size - size) >= META_SIZE + MIN_BLOCK_SIZE) {
        // split the block because there is room for another in the top end
        split_block(block, size);
      }
      block->free = false;
    }
  }
  // return the address right ahead of the block struct
  // (keep in mind this is pointer arithmetic)
  void *p = block + 1;
  debug_print("MALLOC: malloc(%d)     =>  (ptr=%p, size=%d)\n", size, p, block->size);
  return p;
}

void* calloc(size_t num_elems, size_t elem_size) {
  size_t size = num_elems * elem_size; // TODO: check for overflow
  void* ptr = malloc(size);
  debug_print("MALLOC: calloc(%d, %d)  =>  (ptr=%p, size=%d)\n", num_elems, elem_size, ptr, size);
  return memset(ptr, 0, size);
}

// takes a pointer to memory returned to by an *alloc function and walks
// block list in order to find which block the ptr is within.
block_meta_t get_ptr_block(void* ptr) {
  block_meta_t b = global_base;
  while (b->next) {
    if (ptr_within_block(b, ptr)) {
      return b;
    } else {
      b = b->next;
    }
  }
  // no block found that contains pointer (this shouldn't happen)
  return NULL;
}

// either returns the block of the valid pointer OR NULL if the address
// is invalid for any reason
block_meta_t valid_addr(void* p) {
  if (global_base) {
    // malloc has been called at least once
    if (p > global_base && p < sbrk(0)) {
      // pointer is within the heap address range, see if we can find it's block
      return get_ptr_block(p);
    }
  }
  // address was invalid
  return NULL;
}

void free(void* ptr) {

  debug_print("MALLOC: free(%p)\n", ptr);
  if (!ptr) {
    return;
  }

  block_meta_t b = valid_addr(ptr);
  if (b) {
    // ptr was a valid address
    b->free = true;
    // attempt to fuse with previous if possible
    // NOTE: we call fuse_with_next STARTING with the previous block
    if (b->prev && b->prev->free) {
      b = fuse_with_next(b->prev);
    }
    // now see if there is a block after us and attempt to free
    if (b->next) {
      fuse_with_next(b);
    } else {
      // we are the last block in the heap, free the end of the heap
      if (b->prev) {
        b->prev->next = NULL;
      } else {
        // should only execute if we are the last block in the heap
        // so reset the heap break to the bottom of the last block, aka
        // it's original position pre-malloc
        global_base = NULL;
        brk(b);
      }
    }
  }
}

void* realloc(void* ptr, size_t size) {
  if (!ptr) {
    // NULL ptr, realloc should act like malloc
    return malloc(size);
  }

  block_meta_t b = valid_addr(ptr);
  if (b) {
    size_t s = align16(size);
    if (b->size >= s) {
      // we have enough space, see if we can split
      if (b->size - s >= (META_SIZE + MIN_BLOCK_SIZE)) {
        split_block(b, s);
      }

    } else {
      // requested size larger than current block, try to fuse if possible
      if (b->next && b->next->free &&
          (b->size + META_SIZE + b->next->size) >= s) {
        // next block is free and provides big enough space,
        // so fuse them together
        fuse_with_next(b);
        if (b->size - s >= (META_SIZE + MIN_BLOCK_SIZE)) {
          // if the new fused larger block has enough room for it,
          // do a new split
          split_block(b, s);
        }

      } else {
        // next block can't be used because either isn't free or too small,
        // or it doesn't exist because we are at the end of the heap.
        // Just do a standard realloc.
        void* new_ptr = malloc(s);
        if (!new_ptr) {
          // malloc failed, you're doomed
          errno = ENOMEM;
          return NULL;
        }
        memcpy(new_ptr, ptr, s);
        free(ptr);

        debug_print("MALLOC: realloc(%p, %d) =>  (ptr=%p, size=%d)\n", ptr, size, new_ptr, s);
        return new_ptr;
      }
    }

    // here we succeeded fusing with next block or had enough space in
    // original block, so we just give back the original ptr
    return ptr;

  } else {
    // address invalid, can't realloc
    return NULL;
  }
}
