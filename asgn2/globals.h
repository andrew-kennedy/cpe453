#pragma once
#include <stdio.h>

//place defines and prototypes here
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37
#define MAX_ROW 25
#define MAX_COL 80

#define print_fmt_buf(fmt_str, ...) \
  const char *fmt = fmt_str; \
  int sz = snprintf(NULL, 0, fmt, __VA_ARGS__); \
  char buf[sz + 1]; \
  snprintf(buf, sizeof buf, fmt, __VA_ARGS__); \
  print_string(buf)
