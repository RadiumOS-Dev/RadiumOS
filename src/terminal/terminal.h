#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>
#include <stdint.h>
#include "../vga/vga.h"
#include "../utility/utility.h"

// Function prototypes for terminal operations
void terminal_initialize(void);
void terminal_setcolor(uint8_t color);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void print(const char* data);
void print_decimal(int num);
void print_hex(int num);
void print_octal(int num);
void print_integer(int value);
void terminal_clear(void);
void terminal_clear_inFunction(void);
void print_slow(const char* data, uint32_t delay_time);
void print_uint(unsigned int value);
void print_hex_byte(uint8_t value);
void print_capacity(uint64_t bytes);
void print_uint64(uint64_t value);
void print_qemu(char msg[]);
void printr(const char* format, ...);
void terminal_set_cursor_position(size_t position);
void terminal_update_cursor(void);
void psf_init(void);
// New function prototype for setting terminal size
void terminal_setsize(size_t width, size_t height);

#endif // TERMINAL_H
