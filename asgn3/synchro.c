#include "globals.h"
#include "synchro.h"
#include "os.h"
#include <avr/interrupt.h>


void mutex_init(struct mutex_t* m) {
  cli();
  m->ownerId = UINT8_MAX;
  circular_buf_init(&m->waitingThreads, MAX_THREADS);
  m->lock = false;
  sei();
}

/**
 * If the mutex is not locked, the thread locks the mutex and becomes the
 * owner. Otherwise, the thread is placed on the waiting list, put in the
 * waiting state, and the next thread is switched in. Once the thread is
 * no longer waiting, it locks the mutex and becomes the owner.
 *
 * @param m The address of the mutex_t
 */
void mutex_lock(struct mutex_t* m) {
  cli();
  if (m->lock) {
    sys->threads[sys->currentThreadId].state = THREAD_WAITING;
    circular_buf_put(&m->waitingThreads, sys->currentThreadId);
    switch_next_thread();
  }
  m->ownerId = sys->currentThreadId;
  m->lock = true;
  sei();
}

/**
 * If the thread is not the owner, do nothing. Otherwise, unlock the mutex
 * and if there is a thread waiting, remove the first one from the waiting
 * list, change its state to ready, and switch to it.
 *
 * @param m The address of the mutex_t
 */
void mutex_unlock(struct mutex_t* m) {
  cli();
  if (m->ownerId == sys->currentThreadId) {
    // unlock the mutex
    m->lock = false;
    uint8_t nextThreadId = 0;
    if (circular_buf_get(&m->waitingThreads, &nextThreadId) != -1) {
      // got a thread off the waiting list
      sys->threads[nextThreadId].state = THREAD_READY;
      switch_to_thread(nextThreadId);
    }
  }
  sei();
}

void sem_init(struct semaphore_t* s, int8_t value) {
  cli();
  s->value = value;
  circular_buf_init(&s->waitingThreads, MAX_THREADS);
  sei();
}

/**
 * If the semaphore has an available space, decrement the number of free
 * spaces. Otherwise, put the thread on the waiting list, change its state,
 * switch to the next thread, and then decrement the number of free spaces.
 *
 * It seems kind of unfair if I am finally at the front of the waiting line,
 * someone signals, I am ready, someone else grabs my spot before I run, and
 * now I am at the end of the waitlist again.
 *
 * @param s The address of the semaphore_t
 */
void sem_wait(struct semaphore_t* s) {
  cli();
  //  If the value of semaphore variable is not negative, decrement it by 1. If
  //  the semaphore variable is now negative, the process executing wait is
  //  blocked (i.e., added to the semaphore's queue) until the value is greater
  //  or equal to 1. Otherwise, the process continues execution, having used a
  //  unit of the resource.
  while (s->value <= 0) {
    sys->threads[sys->currentThreadId].state = THREAD_WAITING;
    circular_buf_put(&s->waitingThreads, sys->currentThreadId);
    switch_next_thread();
  }
  s->value--;
  sei();
}
// should change the first waiting thread to ready, but continue running in
// the current thread.
// Increments the value of semaphore variable by 1. After the increment, if the
// pre-increment value was negative (meaning there are processes waiting for a
// resource), it transfers a blocked process from the semaphore's waiting queue
// to the ready queue.
void sem_signal(struct semaphore_t* s) {
  cli();
  s->value++;
  uint8_t nextThreadId = 0;
  if (circular_buf_get(&s->waitingThreads, &nextThreadId) != -1) {
    // got a thread off the waiting list
    sys->threads[nextThreadId].state = THREAD_READY;
  }

  sei();
}
// should immediately switch to the first waiting thread (do not wait for
// the next timer interrupt
void sem_signal_swap(struct semaphore_t* s) {
  cli();
  s->value++;
  uint8_t nextThreadId = 0;
  if (circular_buf_get(&s->waitingThreads, &nextThreadId) != -1) {
    // got a thread off the waiting list
    sys->threads[nextThreadId].state = THREAD_READY;
    switch_to_thread(nextThreadId);
  }
  sei();
}
