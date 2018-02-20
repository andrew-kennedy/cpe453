#include "program3.h"
#include "globals.h"
#include "os.h"
#include "serial.h"
#include "synchro.h"
#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/delay.h>

static struct semaphore_t s;
static struct mutex_t m;
static size_t bufSize = 0;

static uint16_t productionRate = DEFAULT_PRODUCE_TICKS;
static uint16_t consumptionRate = DEFAULT_CONSUME_TICKS;
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
void display_stats(void) {
  clear_screen();

  while (true) {

    handleInput();
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
    for (size_t i = 0; i < sys->threadCount; i++) {
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

void blink(void) {
  while (true) {
    if (bufSize < MAX_BUFFER_SIZE && productionRate > 0) {
      led_on();
    } else {
      led_off();
    }
  }
}

int main(int argc, char** argv) {
  serial_init();
  os_init();

  create_thread("producer", (uint16_t)producer, 0, 50);
  create_thread("consumer", (uint16_t)consumer, 0, 51);
  create_thread("stats", (uint16_t)display_stats, 0, 52);
  create_thread("buf_viz", (uint16_t)display_bounded_buffer, 0, 53);
  create_thread("blink", (uint16_t)blink, 0, 54);

  sem_init(&s, 1);
  mutex_init(&m);

  clear_screen();
  os_start();
  sei();

  while (true) {
  }
  return 0;
}

// simulate producing an item and placing an item in the buffer
// default to 1 item per 1000 ms
void producer(void) {
  while (true) {
    thread_sleep(productionRate);

    if (bufSize < MAX_BUFFER_SIZE) {
      mutex_lock(&m);
      sem_wait(&s);

      bufSize++;

      sem_signal(&s);
      mutex_unlock(&m);
    }
  }
}

void consumer(void) {
  while (true) {
    thread_sleep(consumptionRate);

    if (bufSize > 0) {
      mutex_lock(&m);
      sem_wait(&s);

      bufSize--;

      sem_signal(&s);
      mutex_lock(&m);
    }
  }
}

void display_bounded_buffer(void) {
  while (true) {
         set_cursor(1, 81);
      set_color(MAGENTA);
      print_string("Producing 1 item per ");
      print_int(productionRate * 10);
      print_string(" ms   ");

      set_cursor(2, 81);
      print_string("Consuming 1 item per ");
      print_int(consumptionRate * 10);
      print_string(" ms   ");

      for (size_t i = 0; i < MAX_BUFFER_SIZE; i++) {
         set_color(RED + (i % (WHITE - RED)));
         set_cursor(3 + MAX_BUFFER_SIZE - i, 91);

         if (i < bufSize) {
            print_string("[o]");
         }
         else {
            print_string("[ ]");
         }
      }

      thread_sleep(5);
  }
}

void handleInput() {
  uint8_t key = read_byte();
  if (key == 255) {
    return;
  }
  switch (key) {
  case 'a':
    if (productionRate < UINT16_MAX) {
      productionRate++;
    }
    break;
  case 'z':
    if (productionRate > 0) {
      productionRate--;
    }
    break;
  case 'k':
    if (consumptionRate < UINT16_MAX) {
      consumptionRate++;
    }
    break;
  case 'm':
    if (consumptionRate > 0) {
      consumptionRate--;
    }
    break;
  default:
    break;
  }
}
