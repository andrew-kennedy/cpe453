#include <avr/io.h>
#include "globals.h"
#include <stdio.h>


/*
 * Initialize the serial port.
 */
void serial_init() {
  uint16_t baud_setting;

  UCSR0A = _BV(U2X0);
  baud_setting = 16; //115200 baud

  // assign the baud_setting
  UBRR0H = baud_setting >> 8;
  UBRR0L = baud_setting;

  // enable transmit and receive
  UCSR0B |= (1 << TXEN0) | (1 << RXEN0);
}

/*
 * Return 1 if a character is available else return 0.
 */
uint8_t byte_available() {
  return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

/*
 * Unbuffered read
 * Return 255 if no character is available otherwise return available character.
 */
uint8_t read_byte() {
  if (UCSR0A & (1 << RXC0)) return UDR0;
  return 255;
}

/*
 * Unbuffered write
 *
 * b byte to write.
 */
uint8_t write_byte(uint8_t b) {
  //loop until the send buffer is empty
  while (((1 << UDRIE0) & UCSR0B) || !(UCSR0A & (1 << UDRE0))) {}

  //write out the byte
  UDR0 = b;
  return 1;
}

void print_string(char* s) {
  while (*s) {
    write_byte(*s++);
  }
}

void clear_screen() {
  print_string("\e[2J");
}

void print_int(uint16_t i) {
  print_fmt_buf("%d", i);
}

void print_int32(uint32_t i) {
  print_fmt_buf("%d", i);
}

void print_hex(uint16_t i) {
  print_fmt_buf("%#x", i);
}

void print_hex32(uint32_t i) {
  print_fmt_buf("%#x", i);
}

void set_color(uint8_t color) {
  print_fmt_buf("\e[%dm", color);
}

void set_cursor(uint8_t row, uint8_t col) {
  print_fmt_buf("\e[%d;%dH", row, col);
}
