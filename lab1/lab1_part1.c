#include "globals.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void printPosition(uint8_t row, uint8_t col) {
  // prints the current text position in the top left of the terminal
  set_cursor(1, 1);
  print_fmt_buf("(%d, %d)", row, col);
}

void printName(uint8_t row, uint8_t col, char* name) {
  set_cursor(row, col);
  print_string(name);
  set_cursor(row, col);
}

int main(int argc, char const* argv[]) {
  uint8_t m_row = 2, m_col = 1, m_color = WHITE;
  serial_init();
  char* m_name = "Andrew Kennedy";

  printPosition(m_row, m_col);
  printName(m_row, m_col, m_name);

  while (true) {
    uint8_t inputByte = read_byte();
    if (inputByte != 255) {
      switch (inputByte) {
        case 'w':
          if (m_row > 2) {
            m_row--;
          }
          break;
        case 'a':
          if (m_col > 1) {
            m_col--;
          }
          break;
        case 's':
          if (m_row < MAX_ROW) {
            m_row++;
          }
          break;
        case 'd':
          if (m_col <= MAX_COL - strlen(m_name)) {
            m_col++;
          }
          break;
        case 'c':
          if (m_color++ == WHITE) {
            m_color = RED;
          }
          break;
        default: break;
      }
      clear_screen();
      set_color(m_color);
      printPosition(m_row, m_col);
      printName(m_row, m_col, m_name);
    }
  }
}
