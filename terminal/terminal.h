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
int snprintf(char* buffer, size_t size, const char* format, ...);
void terminal_set_cursor_position(size_t position);
void terminal_update_cursor(void);
void psf_init(void);
// New function prototype for setting terminal size
void terminal_setsize(size_t width, size_t height);
void terminal_set_vga_mode(bool enable);
bool terminal_is_vga_mode(void);
vga_window_t* terminal_get_window(void);
void terminal_destroy_vga(void);

// VGA window customization
void terminal_set_window_title(const char* title);
void terminal_set_window_color(enum vga_color fg, enum vga_color bg);
void terminal_enable_border(bool enable);
void terminal_enable_shadow(bool enable);
void terminal_move_window(int x, int y);
void terminal_resize_window(int width, int height);
void terminal_show_window(void);
void terminal_hide_window(void);
// Add to terminal.h
void terminal_flush(void);
void terminal_debug_colors(void);
// Advanced printing
void terminal_print_at(int x, int y, const char* text);
void terminal_print_centered(int y, const char* text);
void terminal_draw_box(int x, int y, int width, int height);
void terminal_fill_rect(int x, int y, int width, int height, char ch);
int sprintf(char* buf, const char* fmt, ...);
// Cursor control
void terminal_get_cursor(int* x, int* y);
void terminal_set_cursor(int x, int y);
void terminal_save_cursor(void);
void terminal_restore_cursor(void);
void vga_init(void);
// Terminal info
void terminal_get_size(int* width, int* height);
void terminal_scroll_up(int lines);
#endif // TERMINAL_H
