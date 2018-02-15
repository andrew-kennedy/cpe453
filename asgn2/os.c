#include "os.h"
#include "globals.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>

#define mask_low(num) 0x00ff & (uint16_t)num
#define mask_high(num) 0x00ff & ((uint16_t)num >> 8)

volatile struct system_t* sys;
static volatile uint32_t isrCounter = 0;

void os_init(void) { sys = calloc(1, sizeof(struct system_t)); }

/**
 * Returns the thread following the currently-executing thread in the system's
 * array list of threads. This is where thread scheduling would be implemented.
 *
 * @return The thread following the thread that is currently executing.
 */
uint8_t get_next_thread(void) {
  return (sys->currentThreadId + 1) % sys->threadCount;
}

/* Context switch will pop off the manually saved registers,
 * then ret to thread_start. ret will pop off the automatically
 * saved registers and thread_start will pop off the
 * function address and then ijmp to the function.
 */
__attribute__((naked)) void context_switch(uint16_t* new_tp, uint16_t* old_tp) {
  // first we have to manually save the remaining registers
  __asm__ volatile("push r2\n\t"
                   "push r3\n\t"
                   "push r3\n\t"
                   "push r4\n\t"
                   "push r5\n\t"
                   "push r6\n\t"
                   "push r7\n\t"
                   "push r8\n\t"
                   "push r9\n\t"
                   "push r10\n\t"
                   "push r11\n\t"
                   "push r12\n\t"
                   "push r13\n\t"
                   "push r14\n\t"
                   "push r15\n\t"
                   "push r16\n\t"
                   "push r17\n\t");

  // time to change the stack stack pointer

  // Load current stack pointer into r16/r17
  __asm__ volatile("in r16, __SP_L__\n\t"
                   "in r17, __SP_H__\n\t");

  // Load the old_tp into z
  // (moves this functions argument 2 into the Z register)
  __asm__ volatile("movw r30, r22\n\t");

  // Save current stack pointer into old_tp using store indirect,
  // so we use the store indirect method to "dereference" the pointer
  __asm__ volatile("st z+, r16\n\t"
                   "st z, r17\n\t");

  // Load new_tp into z
  __asm__ volatile("movw r30, r24\n\t");

  // Load indirect new_tp into r16/r17
  __asm__ volatile("ld r16, z+\n\t"
                   "ld r17, z\n\t");

  // Load new_tp into current stack pointer
  __asm__ volatile("out __SP_L__, r16\n\t"
                   "out __SP_H__, r17\n\t");

  // manually restore all registers
  __asm__ volatile("pop r17\n\t"
                   "pop r16\n\t"
                   "pop r15\n\t"
                   "pop r14\n\t"
                   "pop r13\n\t"
                   "pop r12\n\t"
                   "pop r11\n\t"
                   "pop r10\n\t"
                   "pop r9\n\t"
                   "pop r8\n\t"
                   "pop r7\n\t"
                   "pop r6\n\t"
                   "pop r5\n\t"
                   "pop r4\n\t"
                   "pop r3\n\t"
                   "pop r2\n\t"
                   "ret\n\t");
}

// This interrupt routine is automatically run every 10 milliseconds
ISR(TIMER0_COMPA_vect) {
  // At the beginning of this ISR, the registers r0, r1, and r18-31 have
  // already been pushed to the stack
  // The following statement tells GCC that it can use registers r18-r31
  // for this interrupt routine.  These registers (along with r0 and r1)
  // will automatically be pushed and popped by this interrupt routine.
  __asm__ volatile(""
                   :
                   :
                   : "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25",
                     "r26", "r27", "r30", "r31");
  // Insert your code here
  sys->elapsedTime = isrCounter++ / 100;
  // force context switch
  // Call get_next_thread to get the thread id of the next thread to run
  // Call context switch here to switch to that next thread
  volatile uint8_t nextThreadId = get_next_thread();
  volatile uint8_t currentThreadId = sys->currentThreadId;
  sys->currentThreadId = nextThreadId;
  context_switch((uint16_t*)&sys->threads[nextThreadId].stackPointer,
                 (uint16_t*)&sys->threads[currentThreadId].stackPointer);

  // At the end of this ISR, GCC generated code will pop r18-r31, r1,
  // and r0 before exiting the ISR
}

// Call this to start the system timer interrupt
void start_system_timer(void) {
  TIMSK0 |= _BV(OCIE0A); // interrupt on compare match
  TCCR0A |= _BV(WGM01);  // clear timer on compare match

  // Generate timer interrupt every ~10 milliseconds
  TCCR0B |= _BV(CS02) | _BV(CS00); // prescalar /1024
  OCR0A = 156;                     // generate interrupt every 9.98 milliseconds
}

__attribute__((naked)) void thread_start(void) {
  sei(); // enable interrupts - leave as the first statement in thread_start()
  __asm__ volatile("movw r30, r16\n\t"
                   "movw r24, r14\n\t"
                   "ijmp\n\t");
}

/*
 * Adds a new thread to the system data structure. The new thread allocates
 * space for its stack, and sets its stack bounds, its stack pointer, and
 * its program counter.
 *
 * @param name The name of the thread, 10 char max
 * @param address The address the program counter will assume upon thread start
 * @param args A pointer to a list of arguments passed into the thread
 * @param stackSize The stack size in bytes available to the thread
 */
void create_thread(char* name, uint16_t address, void* args,
                   uint16_t stack_size) {
  if (sys->threadCount == MAX_THREADS) {
    // can't create any more threads
    return;
  }
  volatile struct thread_t* nt = &sys->threads[sys->threadCount];
  // initialize all fields
  nt->threadId = sys->threadCount++;
  strncpy(nt->threadName, name, MAX_NAME_LENGTH);
  nt->stackSize = stack_size + sizeof(struct regs_interrupt) +
                  sizeof(struct regs_context_switch);
  nt->lowestStackAddress = calloc(nt->stackSize, sizeof(uint8_t));
  nt->highestStackAddress = nt->lowestStackAddress + nt->stackSize;
  nt->functionAddress = address;
  // Only initialize nonzero values because of the calloc
  // context switch registers go at HIGH END of stack
  struct regs_context_switch* regs =
      ((struct regs_context_switch*)nt->highestStackAddress) - 1;
  regs->pcl = mask_low(thread_start);
  regs->pch = mask_high(thread_start);
  regs->eind = 0;
  regs->r14 = mask_low(args);
  regs->r15 = mask_high(args);
  regs->r16 = mask_low(address);
  regs->r17 = mask_high(address);

  nt->stackPointer = (uint8_t*)regs;

  return;
}

void os_start(void) {
  uint16_t mainStackPointer = 0;
  start_system_timer();
  sei();
  context_switch((uint16_t*)(&sys->threads[0].stackPointer), &mainStackPointer);
}
