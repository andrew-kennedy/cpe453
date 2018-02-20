#pragma once
#include "circular_buf.h"
#include <stdint.h>

struct mutex_t {
  uint8_t ownerId;
  circular_buf_t waitingThreads;
  bool lock;
};

struct semaphore_t {
  int8_t value;
  circular_buf_t waitingThreads;
};

void mutex_init(struct mutex_t* m);
void mutex_lock(struct mutex_t* m);
void mutex_unlock(struct mutex_t* m);
void sem_init(struct semaphore_t* s, int8_t value);
void sem_wait(struct semaphore_t* s);
void sem_signal(struct semaphore_t* s);
void sem_signal_swap(struct semaphore_t* s);
