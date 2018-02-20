#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct circular_buf_t {
  // fixed size circular buffer
  uint8_t* buffer;
  size_t head;
  size_t tail;
  bool empty;
  size_t size; // of the buffer
} circular_buf_t;

void circular_buf_init(circular_buf_t* cbuf, size_t size);

int8_t circular_buf_reset(circular_buf_t* cbuf);

int8_t circular_buf_put(circular_buf_t* cbuf, uint8_t data);

int8_t circular_buf_get(circular_buf_t* cbuf, uint8_t* data);

bool circular_buf_empty(circular_buf_t cbuf);

bool circular_buf_full(circular_buf_t cbuf);

int8_t circular_buf_destroy(circular_buf_t* cbuf);
