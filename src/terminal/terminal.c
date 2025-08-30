#include "terminal.h"
#include <stddef.h> // For size_t
#include "../utility/utility.h" // For strlen
#include "../timers/timer.h"
#include "../io/io.h"
#include <stdbool.h> // For bool type
#include <stdarg.h>  // For va_list

// Default terminal size
static size_t terminal_width = 80;
static size_t terminal_height = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

void terminal_initialize(void) 
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;
    terminal_clear(); // Clear the terminal on initialization
}

void terminal_set_cursor_position(size_t position) {
    // Set cursor position - low byte first
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    
    // Set cursor position - high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

void terminal_update_cursor(void) {
    size_t position = terminal_row * terminal_width + terminal_column;
    terminal_set_cursor_position(position);
}

void terminal_setsize(size_t width, size_t height) {
    terminal_width = width;
    terminal_height = height;

    // Clear the terminal buffer
    for (size_t y = 0; y < terminal_height; y++) {
        for (size_t x = 0; x < terminal_width; x++) {
            const size_t index = y * terminal_width + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) 
{
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
    const size_t index = y * terminal_width + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
    switch (c) {
        case '\n':
            terminal_column = 0; // Reset column to 0
            if (++terminal_row == terminal_height) {
                terminal_row--; // Prevent overflow
                // Scroll up
                for (size_t y = 1; y < terminal_height; y++) {
                    for (size_t x = 0; x < terminal_width; x++) {
                        terminal_buffer[(y - 1) * terminal_width + x] = terminal_buffer[y * terminal_width + x];
                    }
                }
                // Clear the last row
                for (size_t x = 0; x < terminal_width; x++) {
                    terminal_buffer[(terminal_height - 1) * terminal_width + x] = vga_entry(' ', terminal_color);
                }
            }
            break;

        case '\r':
            // Carriage return - move cursor to beginning of current line
            terminal_column = 0;
            break;

        case '\t':
            // Move to the next tab stop (align to 4-character boundaries)
            terminal_column = (terminal_column + 4) & ~3;
            if (terminal_column >= terminal_width) {
                terminal_column = 0; // Move to the next line if overflow
                if (++terminal_row == terminal_height) {
                    terminal_row--; // Prevent overflow
                    // Scroll up
                    for (size_t y = 1; y < terminal_height; y++) {
                        for (size_t x = 0; x < terminal_width; x++) {
                            terminal_buffer[(y - 1) * terminal_width + x] = terminal_buffer[y * terminal_width + x];
                        }
                    }
                    // Clear the last row
                    for (size_t x = 0; x < terminal_width; x++) {
                        terminal_buffer[(terminal_height - 1) * terminal_width + x] = vga_entry(' ', terminal_color);
                    }
                }
            }
            break;

        case '\b':
            // Handle backspace
            if (terminal_column > 0) {
                terminal_column--; // Move back a column
                terminal_putentryat(' ', terminal_color, terminal_column, terminal_row); // Clear the character
            } else if (terminal_row > 0) {
                terminal_row--; // Move up a row
                terminal_column = terminal_width - 1; // Go to the end of the previous line
                terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
            }
            break;

        default:
            terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
            if (++terminal_column == terminal_width) {
                terminal_column = 0; // Move to the next line if overflow
                if (++terminal_row == terminal_height) {
                    terminal_row--; // Prevent overflow
                    // Scroll up
                    for (size_t y = 1; y < terminal_height; y++) {
                        for (size_t x = 0; x < terminal_width; x++) {
                            terminal_buffer[(y - 1) * terminal_width + x] = terminal_buffer[y * terminal_width + x];
                        }
                    }
                    // Clear the last row
                    for (size_t x = 0; x < terminal_width; x++) {
                        terminal_buffer[(terminal_height - 1) * terminal_width + x] = vga_entry(' ', terminal_color);
                    }
                }
            }
            break;
    }
}

void terminal_write(const char* data, size_t size) 
{
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

void print(const char* data) 
{
    terminal_write(data, strlen(data));
}

// Helper function to convert integer to string with padding and different bases
static void int_to_string_base(long long value, char* buffer, int base, int width, char pad_char, bool uppercase) {
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char temp[64];
    int i = 0;
    bool negative = false;
    
    if (value < 0 && base == 10) {
        negative = true;
        value = -value;
    }
    
    if (value == 0) {
        temp[i++] = '0';
    } else {
        while (value > 0) {
            temp[i++] = digits[value % base];
            value /= base;
        }
    }
    
    // Calculate padding
    int num_len = i + (negative ? 1 : 0);
    int padding = (width > num_len) ? width - num_len : 0;
    int pos = 0;
    
    // Add padding (before number for space padding, after sign for zero padding)
    if (pad_char == ' ') {
        for (int j = 0; j < padding; j++) {
            buffer[pos++] = ' ';
        }
    }
    
    // Add negative sign
    if (negative) {
        buffer[pos++] = '-';
    }
    
    // Add zero padding after sign
    if (pad_char == '0') {
        for (int j = 0; j < padding; j++) {
            buffer[pos++] = '0';
        }
    }
    
    // Add digits (reverse order)
    while (i > 0) {
        buffer[pos++] = temp[--i];
    }
    
    buffer[pos] = '\0';
}

// Main printf function (renamed from printr to printf)
void printr(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    char* buf_ptr = buffer;
    const char* fmt_ptr = format;
    
    while (*fmt_ptr && (buf_ptr - buffer) < 1023) {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1)) {
            fmt_ptr++; // Skip %
            
            // Parse flags and width
            int width = 0;
            char pad_char = ' ';
            bool left_align = false;
            bool force_sign = false;
            bool space_sign = false;
            bool alternate_form = false;
            
            // Parse flags
            while (1) {
                if (*fmt_ptr == '-') {
                    left_align = true;
                    fmt_ptr++;
                } else if (*fmt_ptr == '+') {
                    force_sign = true;
                    fmt_ptr++;
                } else if (*fmt_ptr == ' ') {
                    space_sign = true;
                    fmt_ptr++;
                } else if (*fmt_ptr == '#') {
                    alternate_form = true;
                    fmt_ptr++;
                } else if (*fmt_ptr == '0') {
                    pad_char = '0';
                    fmt_ptr++;
                } else {
                    break;
                }
            }
            
            // Parse width
            while (*fmt_ptr >= '0' && *fmt_ptr <= '9') {
                width = width * 10 + (*fmt_ptr - '0');
                fmt_ptr++;
            }
            
            // Handle format specifiers
            switch (*fmt_ptr) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 10, width, pad_char, false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && (buf_ptr - buffer) < 1023) {
                        *buf_ptr++ = *num_ptr++;
                    }
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 10, width, pad_char, false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && (buf_ptr - buffer) < 1023) {
                        *buf_ptr++ = *num_ptr++;
                    }
                    break;
                }
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    if (alternate_form && value != 0) {
                        *buf_ptr++ = '0';
                        *buf_ptr++ = 'x';
                    }
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 16, width, pad_char, false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && (buf_ptr - buffer) < 1023) {
                        *buf_ptr++ = *num_ptr++;
                    }
                    break;
                }
                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    if (alternate_form && value != 0) {
                        *buf_ptr++ = '0';
                        *buf_ptr++ = 'X';
                    }
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 16, width, pad_char, true);
                    char* num_ptr = num_buf;
                    while (*num_ptr && (buf_ptr - buffer) < 1023) {
                        *buf_ptr++ = *num_ptr++;
                    }
                    break;
                }
                case 'o': {
                    unsigned int value = va_arg(args, unsigned int);
                    if (alternate_form && value != 0) {
                        *buf_ptr++ = '0';
                    }
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 8, width, pad_char, false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && (buf_ptr - buffer) < 1023) {
                        *buf_ptr++ = *num_ptr++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    *buf_ptr++ = c;
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    while (*str && (buf_ptr - buffer) < 1023) {
                        *buf_ptr++ = *str++;
                    }
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    *buf_ptr++ = '0';
                    *buf_ptr++ = 'x';
                    char num_buf[32];
                    int_to_string_base((unsigned long)ptr, num_buf, 16, 8, '0', false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && (buf_ptr - buffer) < 1023) {
                        *buf_ptr++ = *num_ptr++;
                    }
                    break;
                }
                case '%': {
                    *buf_ptr++ = '%';
                    break;
                }
                default: {
                    *buf_ptr++ = '%';
                    *buf_ptr++ = *fmt_ptr;
                    break;
                }
            }
        } else {
            *buf_ptr++ = *fmt_ptr;
        }
        fmt_ptr++;
    }
    
    *buf_ptr = '\0';
    print(buffer);
    
    va_end(args);
}

void print_integer(int value) {
    char buffer[12];
    int index = 0;
    
    if (value < 0) {
        buffer[index++] = '-';
        value = -value;
    }
    
    if (value == 0) {
        buffer[index++] = '0';
    } else {
        char temp[12];
        int temp_index = 0;
        while (value > 0) {
            temp[temp_index++] = (value % 10) + '0';
            value /= 10;
        }
        // Reverse
        for (int i = temp_index - 1; i >= 0; i--) {
            buffer[index++] = temp[i];
        }
    }
    
    buffer[index] = '\0';
    print(buffer);
}

void print_decimal(int num) 
{
    print_integer(num); // Use the fixed print_integer function
}

void print_hex(int num) 
{
    char buffer[9];
    int i = 0;

    if (num == 0) {
        print("0");
        return;
    }

    while (num > 0) {
        int digit = num % 16;
        buffer[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        num /= 16;
    }
    
    buffer[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    print(buffer);
}

void print_octal(int num) 
{
    char buffer[12];
    int i = 0;

    if (num == 0) {
        print("0");
        return;
    }

    while (num > 0) {
        buffer[i++] = (num % 8) + '0';
        num /= 8;
    }
    
    buffer[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    print(buffer);
}

void print_slow(const char* data, uint32_t delay_time) {
    for (size_t i = 0; data[i] != '\0'; i++) {
        terminal_putchar(data[i]);
        delay(delay_time);
    }
    terminal_update_cursor();
}

void print_hex_byte(uint8_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    terminal_putchar(hex_chars[(value >> 4) & 0xF]);
    terminal_putchar(hex_chars[value & 0xF]);
    terminal_update_cursor();
}

void print_uint64(uint64_t value) {
    char buffer[21];
    int i = 0;
    
    if (value == 0) {
        print("0");
        return;
    }
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Reverse the buffer
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
    buffer[i] = '\0';
    
    print(buffer);
}

void print_uint(unsigned int value) {
    char buffer[11]; // Maximum digits for unsigned int (32-bit) is 10 + null terminator
    int i = 0;
    
    if (value == 0) {
        print("0");
        return;
    }
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Reverse the buffer
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
    buffer[i] = '\0';
    
    print(buffer);
}

void print_capacity(uint64_t bytes) {
    if (bytes < 1024) {
        print_uint64(bytes);
        print(" B");
    } else if (bytes < 1024 * 1024) {
        print_uint64(bytes / 1024);
        print(" KB");
    } else if (bytes < 1024ULL * 1024 * 1024) {
        print_uint64(bytes / (1024 * 1024));
        print(" MB");
    } else if (bytes < 1024ULL * 1024 * 1024 * 1024) {
        print_uint64(bytes / (1024ULL * 1024 * 1024));
        print(" GB");
    } else {
        print_uint64(bytes / (1024ULL * 1024 * 1024 * 1024));
        print(" TB");
    }
}

void print_qemu(char msg[]) {
    for(int i = 0; msg[i]; i++) {
        outb(0x3F8, msg[i]); // COM1 serial port
    }
    outb(0x3F8, '\n');
}

void terminal_clear(void) 
{
    for (size_t y = 0; y < terminal_height; y++) {
        for (size_t x = 0; x < terminal_width; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
    
}
