#pragma once
#include <stdint.h>

// default interrupt ticks are every 10 ms
#define DEFAULT_CONSUME_TICKS 100
#define DEFAULT_PRODUCE_TICKS 100
#define MAX_BUFFER_SIZE 10

int main(int argc, char ** argv);
void led_on(void);
void led_off(void);
void blink(void);
void handleInput(void);
void producer(void);
void consumer(void);
void display_stats(void);
void display_bounded_buffer(void);
