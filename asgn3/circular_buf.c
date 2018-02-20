#include "circular_buf.h"
#include <stdlib.h>

void circular_buf_init(circular_buf_t *cbuf, size_t size) {
  cbuf->size = size;
  cbuf->buffer = malloc(cbuf->size);
  circular_buf_reset(cbuf);
}

int8_t circular_buf_reset(circular_buf_t* cbuf) {
  int8_t r = -1;

  if (cbuf) {
    cbuf->head = 0;
    cbuf->empty = true;
    cbuf->tail = 0;
    r = 0;
  }
  return r;
}

int8_t circular_buf_put(circular_buf_t* cbuf, uint8_t data) {

  int8_t r = -1;

  if (cbuf) {
    cbuf->buffer[cbuf->head] = data;
    cbuf->empty = false;
    cbuf->head = (cbuf->head + 1) % cbuf->size;

    if (cbuf->head == cbuf->tail) {
      cbuf->tail = (cbuf->tail + 1) % cbuf->size;
    }

    r = 0;
  }

  return r;
}

int8_t circular_buf_get(circular_buf_t* cbuf, uint8_t* data) {
  int8_t r = -1;

  if (cbuf && data && !circular_buf_empty(*cbuf)) {
    *data = cbuf->buffer[cbuf->tail];
    cbuf->tail = (cbuf->tail + 1) % cbuf->size;
    if (circular_buf_empty(*cbuf)) {
      cbuf->empty = true;
    }

    r = 0;
  }

  return r;
}

bool circular_buf_empty(circular_buf_t cbuf) {
  return ((cbuf.head == cbuf.tail) && cbuf.empty);
}

bool circular_buf_full(circular_buf_t cbuf) {
  return ((cbuf.head == cbuf.tail) && !cbuf.empty);
}

int8_t circular_buf_destroy(circular_buf_t* cbuf) {
  int8_t r = -1;
  if (cbuf) {
    free(cbuf->buffer);
    r = 0;
  }
  return r;
}
