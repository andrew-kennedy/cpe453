#include "globals.h"
#include "os.h"
#include <stdbool.h>
#include <util/delay.h>


/*
 * Prints the following information:
 * 1. System time (in seconds)
 * 2. Number of threads in the system
 * 3. Per-thread information:
 *   - thread id
 *   - thread name
 *   - thread pc (starting pc)
 *   - stack usage (number of bytes used by the stack)
 *   - total stack size (number of bytes allocated for the stack)
 *   - current top of stack (current top of stack address)
 *   - stack base (lowest possible stack address)
 *   - stack end (highest possible stack address)
 */
void printSystemInfo(void) {
  clear_screen();

  while (1) {
    _delay_ms(100);
    set_cursor(1, 1);

    set_color(MAGENTA);

    // System time
    print_string("Elapsed system time: ");
    print_int32(sys->elapsedTime);
    print_string("s\r\n");

    // Number of threads in the system

    print_string("Thread count: ");
    print_int(sys->threadCount);
    print_string("\r\n\n");

    // Per-thread information
    for (int i = 0; i < sys->threadCount; i++) {
      set_color(WHITE);
      print_string("Thread ID: ");
      print_int(sys->threads[i].threadId);
      print_string("\r\n");

      print_string("Thread Name: ");
      print_string(sys->threads[i].threadName);
      print_string("\r\n");

      print_string("Thread PC: ");
      print_hex(sys->threads[i].functionAddress);
      print_string("\r\n");

      print_string("Stack usage: ");
      print_int((uint16_t)(sys->threads[i].highestStackAddress -
                           sys->threads[i].stackPointer));
      print_string("\r\n");

      print_string("Total stack size: ");
      print_int(sys->threads[i].stackSize);
      print_string("\r\n");

      print_string("Current top of stack: ");
      print_hex((uint16_t)sys->threads[i].stackPointer);
      print_string("\r\n");

      print_string("Stack base: ");
      print_hex((uint16_t)sys->threads[i].highestStackAddress);
      print_string("\r\n");

      print_string("Stack end: ");
      print_hex((uint16_t)sys->threads[i].lowestStackAddress);
      print_string("\r\n\n");
    }
  }
}

void led_on(void) {
  // Set data direction to OUTPUT
  // Clear Z high byte
  __asm__ __volatile__("clr r31");
  // Set Z low byte to DDRB
  __asm__ __volatile__("ldi r30, 0x24");
  // Set 0x20 (DDRB bit 5 to 1)
  __asm__ __volatile__("ldi r16, 0x80");
  // Write 0x20 to location 0x24 (set LED pin as output)
  __asm__ __volatile__("st Z, r16");

  // Set LED to ON
  // Set Z low byte to LED register
  __asm__ __volatile__("ldi r30, 0x25");

  __asm__ __volatile__("ldi r16, 0x80");
  __asm__ __volatile__("st Z, r16");
}

void led_off(void) {
  // Set data direction to OUTPUT
  // Clear Z high byte
  __asm__ __volatile__("clr r31");
  // Set Z low byte to DDRB
  __asm__ __volatile__("ldi r30, 0x24");
  // Set 0x20 (DDRB bit 5 to 1)
  __asm__ __volatile__("ldi r16, 0x80");
  // Write 0x20 to location 0x24 (set LED pin as output)
  __asm__ __volatile__("st Z, r16");

  // Set LED to OFF
  __asm__ __volatile__("ldi r30, 0x25");

  __asm__ __volatile__("ldi r16, 0x00");
  __asm__ __volatile__("st Z, r16");
}


void blink(uint16_t blinkRate) {

  volatile uint16_t delayCountdown;

  while (true) {
    delayCountdown = blinkRate;
    led_off();
    while (delayCountdown--) {
      _delay_ms(1);
    }
    led_on();
    delayCountdown = blinkRate;
    while (delayCountdown--) {
      _delay_ms(1);
    }
  }
}

void main(void) {
   serial_init();
   os_init();

   create_thread("print", (uint16_t) printSystemInfo, 0, 500);
   create_thread("blink", (uint16_t) blink, 500, 500);

   os_start();

   while(1) {}
}
