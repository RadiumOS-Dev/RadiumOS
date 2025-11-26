#include "terminal.h"
#include <stddef.h> // For size_t
#include "../utility/utility.h" // For strlen
#include "../timers/timer.h"
#include "../io/io.h"
#include "../commands/cowsay.h"
#include <stdbool.h> // For bool type
#include <stdarg.h>  // For va_list
#include <limits.h> 

// Default terminal size
static size_t terminal_width = 80;
static size_t terminal_height = 25;

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

char _binary_font_psf_start[];
char _binary_font_psf_end[];
typedef struct {
    uint32_t magic;       /* magic bytes to identify PSF */
    uint32_t version;     /* zero */
    uint32_t headersize;  /* offset of bitmaps in file, 32 */
    uint32_t flags;       /* 0 if there's no unicode table */
    uint32_t numglyph;    /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;      /* height in pixels */
    uint32_t width;       /* width in pixels */
} PSF_font;
#define PSF_FONT_MAGIC 0x864ab572
uint16_t* unicode_table = NULL;
void psf_init() {
    uint16_t glyph = 0;
    PSF_font* font = (PSF_font*)_binary_font_psf_start;
    if (font->magic != PSF_FONT_MAGIC) {
        unicode_table = NULL;
        return;
    }
    if (font->flags == 0) {
        unicode_table = NULL;
        return;
    }
    char* s = (char*)(_binary_font_psf_start + font->headersize + font->numglyph * font->bytesperglyph);
    unicode_table = calloc(USHRT_MAX, sizeof(uint16_t));
    while (s < _binary_font_psf_end) {
        uint16_t uc = (uint8_t)s[0];
        if (uc == 0xFF) {
            glyph++;
            s++;
            continue;
        } else if (uc & 128) {
            // UTF-8 to unicode conversion
            if ((uc & 32) == 0) {
                uc = ((s[0] & 0x1F) << 6) + (s[1] & 0x3F);
                s++;
            } else if ((uc & 16) == 0) {
                uc = ((((s[0] & 0xF) << 6) + (s[1] & 0x3F)) << 6) + (s[2] & 0x3F);
                s += 2;
            } else if ((uc & 8) == 0) {
                uc = (((((s[0] & 0x7) << 6) + (s[1] & 0x3F)) << 6) + (s[2] & 0x3F)) << 6 + (s[3] & 0x3F);
                s += 3;
            } else {
                uc = 0;
            }
        }
        unicode_table[uc] = glyph;
        s++;
    }
}

void terminal_initialize(void) 
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    terminal_buffer = (uint16_t*) 0xB8000;
    terminal_clear_inFunction(); // Clear the terminal on initialization
}

void terminal_set_cursor_position(size_t position) {
  //
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
    terminal_update_cursor();
}



// Helper function to convert unsigned integer to string
static int uitoa(unsigned int value, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    unsigned int tmp_value;
    int len = 0;

    // Convert to string (reversed)
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
        len++;
    } while (value);

    *ptr-- = '\0';

    // Reverse the string
    while (str < ptr) {
        tmp_char = *ptr;
        *ptr-- = *str;
        *str++ = tmp_char;
    }

    return len;
}

// Simple string length function
static int strlen_local(const char* str) {
    int len = 0;
    while (str[len])
        len++;
    return len;
}

// Main sprintf implementation
int sprintf(char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char* str = buf;
    const char* ptr;
    
    for (ptr = fmt; *ptr != '\0'; ptr++) {
        if (*ptr != '%') {
            *str++ = *ptr;
            continue;
        }
        
        ptr++; // Move past '%'
        
        // Handle format specifiers
        switch (*ptr) {
            case 'd': // Signed decimal integer
            case 'i': {
                int val = va_arg(args, int);
                char num_buf[32];
                itoa(val, num_buf, 10);
                int len = strlen_local(num_buf);
                for (int i = 0; i < len; i++) {
                    *str++ = num_buf[i];
                }
                break;
            }
            
            case 'u': { // Unsigned decimal integer
                unsigned int val = va_arg(args, unsigned int);
                char num_buf[32];
                uitoa(val, num_buf, 10);
                int len = strlen_local(num_buf);
                for (int i = 0; i < len; i++) {
                    *str++ = num_buf[i];
                }
                break;
            }
            
            case 'x': { // Unsigned hexadecimal (lowercase)
                unsigned int val = va_arg(args, unsigned int);
                char num_buf[32];
                uitoa(val, num_buf, 16);
                int len = strlen_local(num_buf);
                for (int i = 0; i < len; i++) {
                    *str++ = num_buf[i];
                }
                break;
            }
            
            case 'X': { // Unsigned hexadecimal (uppercase)
                unsigned int val = va_arg(args, unsigned int);
                char num_buf[32];
                uitoa(val, num_buf, 16);
                int len = strlen_local(num_buf);
                for (int i = 0; i < len; i++) {
                    char c = num_buf[i];
                    if (c >= 'a' && c <= 'f')
                        c = c - 'a' + 'A';
                    *str++ = c;
                }
                break;
            }
            
            case 's': { // String
                char* s = va_arg(args, char*);
                if (s == NULL)
                    s = "(null)";
                while (*s) {
                    *str++ = *s++;
                }
                break;
            }
            
            case 'c': { // Character
                char c = (char)va_arg(args, int);
                *str++ = c;
                break;
            }
            
            case '%': { // Literal '%'
                *str++ = '%';
                break;
            }
            
            default: { // Unknown format specifier
                *str++ = '%';
                *str++ = *ptr;
                break;
            }
        }
    }
    
    *str = '\0'; // Null terminate
    va_end(args);
    
    return str - buf; // Return number of characters written
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
    terminal_update_cursor();
}

void print_decimal(int num) 
{
    print_integer(num); // Use the fixed print_integer function
    terminal_update_cursor();
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
    terminal_update_cursor();
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
    terminal_update_cursor();
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
    terminal_update_cursor();
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
    terminal_update_cursor();
}

void print_capacity(uint64_t bytes) {
    if (bytes < 1024) {
        print_uint64(bytes);
        print("B");
        terminal_update_cursor();
    } else if (bytes < 1024 * 1024) {
        print_uint64(bytes / 1024);
        print("KB");
        terminal_update_cursor();
    } else if (bytes < 1024ULL * 1024 * 1024) {
        print_uint64(bytes / (1024 * 1024));
        print("MB");
        terminal_update_cursor();
    } else if (bytes < 1024ULL * 1024 * 1024 * 1024) {
        print_uint64(bytes / (1024ULL * 1024 * 1024));
        print("GB");
        terminal_update_cursor();
    } else {
        print_uint64(bytes / (1024ULL * 1024 * 1024 * 1024));
        print("TB");
        terminal_update_cursor();
    }
}

void print_qemu(char msg[]) {
    for(int i = 0; msg[i]; i++) {
        outb(0x3F8, msg[i]); // COM1 serial port
    }
    outb(0x3F8, ' ');
}


void terminal_clear_inFunction(void) 
{
    for (size_t y = 0; y < terminal_height; y++) {
        for (size_t x = 0; x < terminal_width; x++) {
            terminal_putentryat(' ', terminal_color, x, y);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
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
int snprintf(char* buffer, size_t size, const char* format, ...) {
    if (!buffer || size == 0) return 0;
    
    va_list args;
    va_start(args, format);
    
    char* buf_ptr = buffer;
    const char* fmt_ptr = format;
    size_t remaining = size - 1; // Leave space for null terminator
    
    while (*fmt_ptr && remaining > 0) {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1)) {
            fmt_ptr++; // Skip %
            
            // Parse flags and width (simplified version)
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
                    while (*num_ptr && remaining > 0) {
                        *buf_ptr++ = *num_ptr++;
                        remaining--;
                    }
                    break;
                }
                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 10, width, pad_char, false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && remaining > 0) {
                        *buf_ptr++ = *num_ptr++;
                        remaining--;
                    }
                    break;
                }
                case 'x': {
                    unsigned int value = va_arg(args, unsigned int);
                    if (alternate_form && value != 0 && remaining >= 2) {
                        *buf_ptr++ = '0';
                        *buf_ptr++ = 'x';
                        remaining -= 2;
                    }
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 16, width, pad_char, false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && remaining > 0) {
                        *buf_ptr++ = *num_ptr++;
                        remaining--;
                    }
                    break;
                }
                case 'X': {
                    unsigned int value = va_arg(args, unsigned int);
                    if (alternate_form && value != 0 && remaining >= 2) {
                        *buf_ptr++ = '0';
                        *buf_ptr++ = 'X';
                        remaining -= 2;
                    }
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 16, width, pad_char, true);
                    char* num_ptr = num_buf;
                    while (*num_ptr && remaining > 0) {
                        *buf_ptr++ = *num_ptr++;
                        remaining--;
                    }
                    break;
                }
                case 'o': {
                    unsigned int value = va_arg(args, unsigned int);
                    if (alternate_form && value != 0 && remaining > 0) {
                        *buf_ptr++ = '0';
                        remaining--;
                    }
                    char num_buf[32];
                    int_to_string_base(value, num_buf, 8, width, pad_char, false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && remaining > 0) {
                        *buf_ptr++ = *num_ptr++;
                        remaining--;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    if (remaining > 0) {
                        *buf_ptr++ = c;
                        remaining--;
                    }
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    if (!str) str = "(null)";
                    while (*str && remaining > 0) {
                        *buf_ptr++ = *str++;
                        remaining--;
                    }
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    if (remaining >= 2) {
                        *buf_ptr++ = '0';
                        *buf_ptr++ = 'x';
                        remaining -= 2;
                    }
                    char num_buf[32];
                    int_to_string_base((unsigned long)ptr, num_buf, 16, 8, '0', false);
                    char* num_ptr = num_buf;
                    while (*num_ptr && remaining > 0) {
                        *buf_ptr++ = *num_ptr++;
                        remaining--;
                    }
                    break;
                }
                case '%': {
                    if (remaining > 0) {
                        *buf_ptr++ = '%';
                        remaining--;
                    }
                    break;
                }
                default: {
                    if (remaining > 1) {
                        *buf_ptr++ = '%';
                        *buf_ptr++ = *fmt_ptr;
                        remaining -= 2;
                    }
                    break;
                }
            }
        } else {
            *buf_ptr++ = *fmt_ptr;
            remaining--;
        }
        fmt_ptr++;
    }
    
    *buf_ptr = '\0';
    va_end(args);
    
    // Return the number of characters that would have been written
    // if buffer was large enough (not including null terminator)
    return (int)(buf_ptr - buffer);
}


/*

% This is a comment !
% the following code runs soon as a new terminal/boot opens up
clear
radifetch
echo "Hello, User !"


*/