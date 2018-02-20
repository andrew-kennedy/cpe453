#pragma once
#include <stdint.h>

void serial_init(void);
uint8_t byte_available(void);
uint8_t read_byte(void);
uint8_t write_byte(uint8_t b);
void print_string(char* s);
void clear_screen(void);
void print_int(uint16_t i);
void print_int32(uint32_t i);
void print_hex(uint16_t i);
void print_hex32(uint32_t i);
void set_color(uint8_t color);
void set_cursor(uint8_t row, uint8_t col);
