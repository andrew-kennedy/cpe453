#include "globals.h"
#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>

__volatile__ uint8_t delayMultiplier = 1;
__volatile__ uint8_t delayCountdown, inputByte;

int main(int argc, char const* argv[]) {

  serial_init();

  while (true) {
    delayCountdown = delayMultiplier;
    led_off();
    while (delayCountdown--) {
      if ((inputByte = read_byte()) != 255) {
        handleInput();
      }
      _delay_ms(50);
    }
    led_on();
    delayCountdown = delayMultiplier;
    while (delayCountdown--) {
      if ((inputByte = read_byte()) != 255) {
        handleInput();
      }
      _delay_ms(50);
    }
  }
}

void handleInput() {
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

void led_on() {
  // Set data direction to OUTPUT
  // Clear Z high byte
  __asm__ __volatile__("clr r31");
  // Set Z low byte to DDRB
  __asm__ __volatile__("ldi r30, 0x24");
  // Set 0x20 (DDRB bit 5 to 1)
  __asm__ __volatile__("ldi r18, 0x80");
  // Write 0x20 to location 0x24 (set LED pin as output)
  __asm__ __volatile__("st Z, r18");

  // Set LED to ON
  // Set Z low byte to LED register
  __asm__ __volatile__("ldi r30,0x25");

  __asm__ __volatile__("ldi r18,0x80");
  __asm__ __volatile__("st Z, r18");
}

void led_off() {
  // Set data direction to OUTPUT
  // Clear Z high byte
  __asm__ __volatile__("clr r31");
  // Set Z low byte to DDRB
  __asm__ __volatile__("ldi r30, 0x24");
  // Set 0x20 (DDRB bit 5 to 1)
  __asm__ __volatile__("ldi r18, 0x80");
  // Write 0x20 to location 0x24 (set LED pin as output)
  __asm__ __volatile__("st Z, r18");

  // Set LED to OFF
  __asm__ __volatile__("ldi r30, 0x25");

  __asm__ __volatile__("ldi r18, 0x00");
  __asm__ __volatile__("st Z, r18");
}
