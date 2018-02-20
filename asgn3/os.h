#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// This structure defines the register order pushed to the stack on a
// system context switch.
struct regs_context_switch {
  // stack pointer is pointing to 1 byte below the top of the stack
  uint8_t padding;

  // Registers that will be managed by the context switch function
  uint8_t r17;
  uint8_t r16;
  uint8_t r15;
  uint8_t r14;
  uint8_t r13;
  uint8_t r12;
  uint8_t r11;
  uint8_t r10;
  uint8_t r9;
  uint8_t r8;
  uint8_t r7;
  uint8_t r6;
  uint8_t r5;
  uint8_t r4;
  uint8_t r3;
  uint8_t r2;
  uint8_t eind; // third byte of the PC
  uint8_t pch;
  uint8_t pcl;
};

// This structure defines how registers are pushed to the stack when
// the system 10ms interrupt occurs.  This struct is never directly
// used, but instead be sure to account for the size of this struct
// when allocating initial stack space
struct regs_interrupt {
  // stack pointer is pointing to 1 byte below the top of the stack
  uint8_t padding;

  // Registers that are pushed to the stack during an interrupt service routine
  uint8_t r31;
  uint8_t r30;
  uint8_t r29;
  uint8_t r28;
  uint8_t r27;
  uint8_t r26;
  uint8_t r25;
  uint8_t r24;
  uint8_t r23;
  uint8_t r22;
  uint8_t r21;
  uint8_t r20;
  uint8_t r19;
  uint8_t r18;

  // RAMPZ and SREG are 2 other state registers in the AVR architecture
  uint8_t rampz; // RAMPZ register
  uint8_t sreg;  // status register

  uint8_t r0;
  uint8_t r1;
  uint8_t eind;
  uint8_t pch;
  uint8_t pcl;
};

// 7 "regular threads" allowed bc there is an implicit main thread
#define MAX_THREADS 8
#define MAIN_THREAD_ID MAX_THREADS - 1
#define MAX_NAME_LENGTH 10
// yields the next threadId in a looping sequence
#define next_thread_id() (sys->currentThreadId + 1) % sys->threadCount

enum thread_state {
  THREAD_RUNNING = 0,
  THREAD_READY,
  THREAD_SLEEPING,
  THREAD_WAITING
};

typedef enum thread_state thread_state;
// This structure holds thread specific information
struct thread_t {
  uint8_t threadId;
  char threadName[MAX_NAME_LENGTH + 1];
  uint16_t stackSize;
  uint16_t functionAddress;
  uint8_t* lowestStackAddress;
  void* highestStackAddress;
  void* stackPointer;
  thread_state state;
  uint16_t schedCount; // number of times per second thread is run
  uint16_t prevSchedCount;
  uint16_t sleepTicks;
};

// This structure holds system information
struct system_t {
  struct thread_t threads[MAX_THREADS];
  uint8_t currentThreadId;
  uint8_t threadCount;
  uint32_t elapsedTime;
};



extern volatile struct system_t* sys;
extern volatile uint32_t isrCounterMilli;
extern volatile uint32_t isrCounterSec;

void init_main_thread(void);
void os_init(void);
uint8_t next_thread_in_seq();
uint8_t get_next_thread(void);
void update_sleeping_threads(void);
void thread_sleep(uint16_t ticks);
void yield(void);
void create_thread(char* name, uint16_t address, void* args,
                   uint16_t stack_size);

void os_start(void);
void switch_to_thread(uint8_t threadId);
void switch_next_thread(void);
