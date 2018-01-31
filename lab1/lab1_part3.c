#include "globals.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>

int main(int argc, char const* argv[]) {
  volatile uint8_t delayMultiplier = 1;
  volatile uint8_t delayCountdown, inputByte;

  serial_init();

  while (true) {
    delayCountdown = delayMultiplier;
    led_off();
    while (delayCountdown--) {
      _delay_ms(500);
    }
    led_on();
    delayCountdown = delayMultiplier;
    while (delayCountdown--) {
      _delay_ms(500);
    }
    inputByte = read_byte();
    if (inputByte != 255) {
      switch (inputByte) {
      case 'z':
        if (delayMultiplier < UINT8_MAX) {
          delayMultiplier++;
        }
        break;
      case 'a':
        if (delayMultiplier > 1) {
          delayMultiplier--;
        }
        break;
      default:
        break;
      }
    }
  }
}


// Load the address of DDRB and PORTB into the Z register, then set the
// 7th bit of each to turn off the LED
void led_on() {
   asm volatile ("ldi r31, 0x0");
   asm volatile ("ldi r30, 0x24");
   asm volatile ("ld r17, z");
   asm volatile ("andi r17, 0x10");
   asm volatile ("st z, r17");

   asm volatile ("ldi r31, 0x0");
   asm volatile ("ldi r30, 0x25");
   asm volatile ("ld r17, z");
   asm volatile ("andi r17, 0x10");
   asm volatile ("st z, r17");
}

// Load the address of DDRB and PORTB into the Z register, then clear the
// 7th bit of each to turn off the LED
void led_off() {
   asm volatile ("ldi r31, 0x0");
   asm volatile ("ldi r30, 0x24");
   asm volatile ("clr r17");
   asm volatile ("st z, r17");

   asm volatile ("ldi r31, 0x0");
   asm volatile ("ldi r30, 0x25");
   asm volatile ("clr r17");
   asm volatile ("st z, r17");
}
