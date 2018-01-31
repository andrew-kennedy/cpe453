#include "globals.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>

int main(int argc, char const* argv[]) {
  uint8_t m_row = 2, m_col = 1, m_color = WHITE;
  uint16_t hexShort = 0, decShort = 0;
  uint32_t hexInt = 0, decInt = 65536;
  bool incrementNumbers = true;

  serial_init();

  while (true) {
    _delay_ms(500);
    uint8_t inputByte = read_byte();
    if (inputByte != 255) {
      incrementNumbers = !incrementNumbers;
    }
    if (incrementNumbers) {
      clear_screen();

      set_cursor(1, 1);
      print_int(decShort++);

      set_cursor(2, 1);
      print_int32(decInt);
      decInt += 1000;

      set_cursor(3, 1);
      print_hex(hexShort++);

      set_cursor(4, 1);
      hexInt += 1000;
      print_hex32(hexInt);
    }
  }
}
