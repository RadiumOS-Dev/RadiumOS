// vga_advanced.c - Complete Advanced VGA Implementation (Part 1/3)
#include "vga.h"
#include "../utility/utility.h"
#include "../keyboard/keyboard.h"
#include "../io/io.h"
#include "../timers/timer.h"

static uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;
#define MAX_WINDOW_BUFFER_SIZE (80 * 25)  // Maximum window size
// Static buffer pool
static uint16_t window_buffers[4][MAX_WINDOW_BUFFER_SIZE];  // Support 4 windows
static bool buffer_in_use[4] = {false, false, false, false};
// Allocate a static buffer
static uint16_t* allocate_window_buffer(int size) {
    if (size > MAX_WINDOW_BUFFER_SIZE) {
        return NULL;
    }
    
    for (int i = 0; i < 4; i++) {
        if (!buffer_in_use[i]) {
            buffer_in_use[i] = true;
            return window_buffers[i];
        }
    }
    
    return NULL;  // No free buffers
}
// Free a static buffer
static void free_window_buffer(uint16_t* buffer) {
    for (int i = 0; i < 4; i++) {
        if (window_buffers[i] == buffer) {
            buffer_in_use[i] = false;
            return;
        }
    }
}
// ===== CORE VGA FUNCTIONS =====

uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | ((uint16_t)color << 8);
}

// ===== WINDOW MANAGEMENT =====

vga_window_t vga_create_window(int x, int y, int w, int h, enum vga_color fg, enum vga_color bg) {
    vga_window_t win;
    
    // Bounds checking
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > VGA_WIDTH) w = VGA_WIDTH - x;
    if (y + h > VGA_HEIGHT) h = VGA_HEIGHT - y;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    
    win.x = x;
    win.y = y;
    win.width = w;
    win.height = h;
    win.color = vga_entry_color(fg, bg);
    win.visible = true;
    win.has_border = false;
    win.has_shadow = false;
    win.title[0] = '\0';
    
    // Try to allocate buffer
    int buffer_size = w * h;
    win.buffer = allocate_window_buffer(buffer_size);
    
    if (win.buffer) {
        // Fill buffer with blank spaces
        uint16_t fill = vga_entry(' ', win.color);
        for (int i = 0; i < buffer_size; i++) {
            win.buffer[i] = fill;
        }
    } else {
        // Allocation failed - mark window as invalid
        win.width = 0;
        win.height = 0;
        win.visible = false;
        
        // Show error on screen
        uint16_t* vga = (uint16_t*)0xB8000;
        const char* err = "ERROR: No free window buffers!";
        uint8_t color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
        for (int i = 0; err[i] != '\0'; i++) {
            vga[VGA_WIDTH * 2 + i] = vga_entry(err[i], color);
        }
    }
    
    return win;
}
vga_window_t vga_create_centered_window(int w, int h, enum vga_color fg, enum vga_color bg) {
    int x = (VGA_WIDTH - w) / 2;
    int y = (VGA_HEIGHT - h) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (w > VGA_WIDTH) w = VGA_WIDTH;
    if (h > VGA_HEIGHT) h = VGA_HEIGHT;
    
    vga_window_t win = vga_create_window(x, y, w, h, fg, bg);
    win.has_border = true;
    win.has_shadow = true;
    
    // IMPORTANT: Make sure color is set correctly
    win.color = vga_entry_color(fg, bg);
    
    return win;
}


void vga_destroy_window(vga_window_t* win) {
    if (!win) return;
    
    // Clear window area on screen with black
    uint16_t blank = vga_entry(' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    for (int row = 0; row < win->height; row++) {
        int screen_y = win->y + row;
        if (screen_y < 0 || screen_y >= VGA_HEIGHT) continue;
        for (int col = 0; col < win->width; col++) {
            int screen_x = win->x + col;
            if (screen_x < 0 || screen_x >= VGA_WIDTH) continue;
            VGA_MEMORY[screen_y * VGA_WIDTH + screen_x] = blank;
        }
    }
    
    // Free buffer
    if (win->buffer) {
        free_window_buffer(win->buffer);
        win->buffer = NULL;
    }
}

void vga_win_clear(vga_window_t* win) {
    if (!win || !win->buffer) return;
    
    // Use the window's current color, not a default
    uint16_t fill = vga_entry(' ', win->color);
    
    for (int i = 0; i < win->width * win->height; i++) {
        win->buffer[i] = fill;
    }
}

void vga_win_refresh(vga_window_t* win) {
    if (!win || !win->buffer || !win->visible) {
        return;
    }
    
    // Draw shadow first if enabled
    if (win->has_shadow) {
        uint8_t shadow_color = vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
        
        // Shadow below (optimized - single loop)
        int sy = win->y + win->height;
        if (sy >= 0 && sy < VGA_HEIGHT) {
            for (int i = 1; i <= win->width && (win->x + i) < VGA_WIDTH; i++) {
                int sx = win->x + i;
                if (sx >= 0) {
                    VGA_MEMORY[sy * VGA_WIDTH + sx] = vga_entry(0xB0, shadow_color);
                }
            }
        }
        
                // Shadow to the right (optimized - single loop)
        int sx = win->x + win->width;
        if (sx >= 0 && sx < VGA_WIDTH) {
            for (int j = 1; j <= win->height && (win->y + j) < VGA_HEIGHT; j++) {
                int sy_right = win->y + j;
                if (sy_right >= 0) {
                    VGA_MEMORY[sy_right * VGA_WIDTH + sx] = vga_entry(0xB0, shadow_color);
                }
            }
        }
    }
    
    // Draw window content from buffer (optimized - memcpy per row)
    for (int row = 0; row < win->height; row++) {
        int screen_y = win->y + row;
        if (screen_y < 0 || screen_y >= VGA_HEIGHT) continue;
        
        int start_col = (win->x < 0) ? -win->x : 0;
        int end_col = win->width;
        if (win->x + end_col > VGA_WIDTH) end_col = VGA_WIDTH - win->x;
        
        if (start_col < end_col) {
            int screen_x = win->x + start_col;
            int buffer_index = row * win->width + start_col;
            int screen_index = screen_y * VGA_WIDTH + screen_x;
            int copy_count = end_col - start_col;
            
            // Fast memory copy for the row
            for (int i = 0; i < copy_count; i++) {
                VGA_MEMORY[screen_index + i] = win->buffer[buffer_index + i];
            }
        }
    }
    
    // Draw border if enabled
    if (win->has_border) {
        uint8_t border_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
        
        // Top border
        if (win->y >= 0 && win->y < VGA_HEIGHT) {
            for (int x = 0; x < win->width; x++) {
                int screen_x = win->x + x;
                if (screen_x >= 0 && screen_x < VGA_WIDTH) {
                    VGA_MEMORY[win->y * VGA_WIDTH + screen_x] = vga_entry(0xCD, border_color);
                }
            }
        }
        
        // Bottom border
        int bottom_y = win->y + win->height - 1;
        if (bottom_y >= 0 && bottom_y < VGA_HEIGHT) {
            for (int x = 0; x < win->width; x++) {
                int screen_x = win->x + x;
                if (screen_x >= 0 && screen_x < VGA_WIDTH) {
                    VGA_MEMORY[bottom_y * VGA_WIDTH + screen_x] = vga_entry(0xCD, border_color);
                }
            }
        }
        
        // Left border
        if (win->x >= 0 && win->x < VGA_WIDTH) {
            for (int y = 0; y < win->height; y++) {
                int screen_y = win->y + y;
                if (screen_y >= 0 && screen_y < VGA_HEIGHT) {
                    VGA_MEMORY[screen_y * VGA_WIDTH + win->x] = vga_entry(0xBA, border_color);
                }
            }
        }
        
        // Right border
        int right_x = win->x + win->width - 1;
        if (right_x >= 0 && right_x < VGA_WIDTH) {
            for (int y = 0; y < win->height; y++) {
                int screen_y = win->y + y;
                if (screen_y >= 0 && screen_y < VGA_HEIGHT) {
                    VGA_MEMORY[screen_y * VGA_WIDTH + right_x] = vga_entry(0xBA, border_color);
                }
            }
        }
        
        // Corners
        if (win->x >= 0 && win->x < VGA_WIDTH && win->y >= 0 && win->y < VGA_HEIGHT) {
            VGA_MEMORY[win->y * VGA_WIDTH + win->x] = vga_entry(0xC9, border_color);
        }
        if (right_x >= 0 && right_x < VGA_WIDTH && win->y >= 0 && win->y < VGA_HEIGHT) {
            VGA_MEMORY[win->y * VGA_WIDTH + right_x] = vga_entry(0xBB, border_color);
        }
        if (win->x >= 0 && win->x < VGA_WIDTH && bottom_y >= 0 && bottom_y < VGA_HEIGHT) {
            VGA_MEMORY[bottom_y * VGA_WIDTH + win->x] = vga_entry(0xC8, border_color);
        }
        if (right_x >= 0 && right_x < VGA_WIDTH && bottom_y >= 0 && bottom_y < VGA_HEIGHT) {
            VGA_MEMORY[bottom_y * VGA_WIDTH + right_x] = vga_entry(0xBC, border_color);
        }
    }
    
    // Draw title if present
    if (win->title[0] != '\0' && win->has_border) {
        int title_len = strlen(win->title);
        int title_x = (win->width - title_len - 2) / 2;
        if (title_x < 1) title_x = 1;
        
        uint8_t title_color = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE);
        
        for (int i = 0; i < title_len && title_x + i < win->width - 1; i++) {
            int screen_x = win->x + title_x + i;
            int screen_y = win->y;
            if (screen_x >= 0 && screen_x < VGA_WIDTH && screen_y >= 0 && screen_y < VGA_HEIGHT) {
                VGA_MEMORY[screen_y * VGA_WIDTH + screen_x] = vga_entry(win->title[i], title_color);
            }
        }
    }
}

void vga_win_set_title(vga_window_t* win, const char* title) {
    if (!win || !title) return;
    strncpy(win->title, title, 63);
    win->title[63] = '\0';
}

void vga_win_show(vga_window_t* win) {
    if (!win) return;
    win->visible = true;
    vga_win_refresh(win);
}

void vga_win_hide(vga_window_t* win) {
    if (!win) return;
    win->visible = false;
    vga_destroy_window(win);
}

void vga_win_move(vga_window_t* win, int new_x, int new_y) {
    if (!win) return;
    
    // Clear old position
    vga_destroy_window(win);
    
    // Update position
    win->x = new_x;
    win->y = new_y;
    
    // Redraw at new position
    vga_win_refresh(win);
}

void vga_win_resize(vga_window_t* win, int new_w, int new_h) {
    if (!win) return;
    
    // Allocate new buffer
    uint16_t* new_buffer = (uint16_t*)malloc(sizeof(uint16_t) * new_w * new_h);
    if (!new_buffer) return;
    
    // Copy old content
    uint16_t fill = vga_entry(' ', win->color);
    for (int i = 0; i < new_w * new_h; i++) {
        new_buffer[i] = fill;
    }
    
    int copy_w = (new_w < win->width) ? new_w : win->width;
    int copy_h = (new_h < win->height) ? new_h : win->height;
    
    for (int y = 0; y < copy_h; y++) {
        for (int x = 0; x < copy_w; x++) {
            new_buffer[y * new_w + x] = win->buffer[y * win->width + x];
        }
    }
    
    // Free old buffer
    free(win->buffer);
    
    // Update window
    win->buffer = new_buffer;
    win->width = new_w;
    win->height = new_h;
    
    vga_win_refresh(win);
}

// ===== DRAWING FUNCTIONS =====

void vga_win_putc(vga_window_t* win, int wx, int wy, char c) {
    if (!win || !win->buffer) return;
    if (wx < 0 || wy < 0 || wx >= win->width || wy >= win->height) return;
    
    // Use the window's color
    win->buffer[wy * win->width + wx] = vga_entry(c, win->color);
}


void vga_win_putc_colored(vga_window_t* win, int wx, int wy, char c, uint8_t color) {
    if (!win || !win->buffer) return;
    if (wx < 0 || wy < 0 || wx >= win->width || wy >= win->height) return;
    win->buffer[wy * win->width + wx] = vga_entry(c, color);
}

void vga_win_puts(vga_window_t* win, int wx, int wy, const char* str) {
    if (!win || !win->buffer || !str) return;
    int x = wx;
    int y = wy;
    int i = 0;
    while (str[i]) {
        if (str[i] == '\n') {
            x = wx;
            y++;
            if (y >= win->height) break;
        } else if (str[i] == '\t') {
            x += 4 - (x % 4);
            if (x >= win->width) {
                x = wx;
                y++;
            }
        } else {
            if (x >= win->width) {
                x = wx;
                y++;
                if (y >= win->height) break;
            }
            vga_win_putc(win, x, y, str[i]);
            x++;
        }
        i++;
    }
}

void vga_win_puts_colored(vga_window_t* win, int wx, int wy, const char* str, uint8_t color) {
    if (!win || !win->buffer || !str) return;
    uint8_t old_color = win->color;
    win->color = color;
    vga_win_puts(win, wx, wy, str);
    win->color = old_color;
}

void vga_win_puts_centered(vga_window_t* win, int y, const char* str) {
    if (!win || !str) return;
    int len = strlen(str);
    int x = (win->width - len) / 2;
    if (x < 0) x = 0;
    vga_win_puts(win, x, y, str);
}

void vga_win_puts_centered_offset(vga_window_t* win, const char* str, int y_offset) {
    if (!win || !win->buffer || !str) return;
    
    int str_len = strlen(str);
    int center_x = (win->width - str_len) / 2;
    int center_y = (win->height / 2) + y_offset;
    
    if (center_x < 0) center_x = 0;
    if (center_y < 0) center_y = 0;
    if (center_y >= win->height) center_y = win->height - 1;
    
    vga_win_puts(win, center_x, center_y, str);
}

void vga_win_draw_box(vga_window_t* win, int x, int y, int w, int h) {
    if (!win || !win->buffer) return;
    if (x < 0 || y < 0 || w <= 0 || h <= 0) return;
    if (x + w > win->width) w = win->width - x;
    if (y + h > win->height) h = win->height - y;
    
    const unsigned char top_left = 0xC9;
    const unsigned char top_right = 0xBB;
    const unsigned char bottom_left = 0xC8;
    const unsigned char bottom_right = 0xBC;
    const unsigned char horizontal = 0xCD;
    const unsigned char vertical = 0xBA;
    
    // Corners
    vga_win_putc(win, x, y, top_left);
    vga_win_putc(win, x + w - 1, y, top_right);
    vga_win_putc(win, x, y + h - 1, bottom_left);
    vga_win_putc(win, x + w - 1, y + h - 1, bottom_right);
    
    // Top and bottom
    for (int i = 1; i < w - 1; i++) {
        vga_win_putc(win, x + i, y, horizontal);
        vga_win_putc(win, x + i, y + h - 1, horizontal);
    }
    
    // Sides
    for (int j = 1; j < h - 1; j++) {
        vga_win_putc(win, x, y + j, vertical);
        vga_win_putc(win, x + w - 1, y + j, vertical);
    }
}

void vga_win_draw_box_colored(vga_window_t* win, int x, int y, int w, int h, uint8_t color) {
    uint8_t old_color = win->color;
    win->color = color;
    vga_win_draw_box(win, x, y, w, h);
    win->color = old_color;
}

void vga_win_fill_rect(vga_window_t* win, int x, int y, int w, int h, char fill_char, uint8_t color) {
    if (!win || !win->buffer) return;
    
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int px = x + col;
            int py = y + row;
            if (px >= 0 && px < win->width && py >= 0 && py < win->height) {
                win->buffer[py * win->width + px] = vga_entry(fill_char, color);
            }
        }
    }
}

void vga_win_draw_line_h(vga_window_t* win, int x, int y, int length, char ch) {
    if (!win || !win->buffer) return;
    for (int i = 0; i < length; i++) {
        if (x + i >= 0 && x + i < win->width && y >= 0 && y < win->height) {
            vga_win_putc(win, x + i, y, ch);
        }
    }
}

void vga_win_draw_line_v(vga_window_t* win, int x, int y, int length, char ch) {
    if (!win || !win->buffer) return;
    for (int i = 0; i < length; i++) {
        if (x >= 0 && x < win->width && y + i >= 0 && y + i < win->height) {
            vga_win_putc(win, x, y + i, ch);
        }
    }
}

void vga_win_draw_shadow(vga_window_t* win, int x, int y, int w, int h) {
    if (!win) return;
    
    uint8_t shadow_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    
    // Shadow below
    for (int i = 1; i <= w; i++) {
        int sx = win->x + x + i;
        int sy = win->y + y + h;
        if (sx >= 0 && sx < VGA_WIDTH && sy >= 0 && sy < VGA_HEIGHT) {
            VGA_MEMORY[sy * VGA_WIDTH + sx] = vga_entry(0xB0, shadow_color);
        }
    }
    
    // Shadow to the right
    for (int j = 1; j <= h; j++) {
        int sx = win->x + x + w;
        int sy = win->y + y + j;
        if (sx >= 0 && sx < VGA_WIDTH && sy >= 0 && sy < VGA_HEIGHT) {
            VGA_MEMORY[sy * VGA_WIDTH + sx] = vga_entry(0xB0, shadow_color);
        }
    }
}

// vga_advanced_part2.c - Buttons, Progress Bars, Textboxes, Menus

// ===== BUTTON FUNCTIONS =====

vga_button_t vga_create_button(int x, int y, int w, int h, const char* text) {
    vga_button_t btn;
    btn.x = x;
    btn.y = y;
    btn.width = w;
    btn.height = h;
    btn.enabled = true;
    btn.selected = false;
    btn.on_click = NULL;
    strncpy(btn.text, text, 31);
    btn.text[31] = '\0';
    return btn;
}

void vga_win_draw_button(vga_window_t* win, int x, int y, int w, int h, const char* text, bool selected, int frame_counter) {
    if (!win || !win->buffer || !text) return;
    if (x < 0 || y < 0 || w <= 2 || h <= 2) return;
    if (x + w + 1 > win->width) w = win->width - x - 1;
    if (y + h + 1 > win->height) h = win->height - y - 1;

    // Draw shadow
    uint8_t shadow_color = vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int sx = x + i + 1;
            int sy = y + j + 1;
            if (sx < win->width && sy < win->height) {
                win->buffer[sy * win->width + sx] = vga_entry(0xB0, shadow_color);
            }
        }
    }

    // Animated selection
    uint8_t button_color;
    if (selected) {
        // Pulse animation
        int phase = (frame_counter / 5) % 8;
        if (phase < 4) {
            button_color = vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_RED);
        } else {
            button_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED);
        }
    } else {
        button_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    }

    // Draw button border
    vga_win_draw_box_colored(win, x, y, w, h, button_color);

    // Fill button
    for (int i = 1; i < w - 1; i++) {
        for (int j = 1; j < h - 1; j++) {
            win->buffer[(y + j) * win->width + (x + i)] = vga_entry(' ', button_color);
        }
    }

    // Center text
    int text_len = strlen(text);
    int text_x = x + (w - text_len) / 2;
    int text_y = y + (h / 2);
    
    for (int i = 0; i < text_len && text_x + i < x + w - 1; i++) {
        win->buffer[text_y * win->width + text_x + i] = vga_entry(text[i], button_color);
    }
}

void vga_button_draw(vga_window_t* win, vga_button_t* btn, int frame_counter) {
    if (!btn || !btn->enabled) return;
    vga_win_draw_button(win, btn->x, btn->y, btn->width, btn->height, btn->text, btn->selected, frame_counter);
}

void vga_button_set_callback(vga_button_t* btn, void (*callback)(void)) {
    if (!btn) return;
    btn->on_click = callback;
}

bool vga_button_is_hovered(vga_button_t* btn, int mouse_x, int mouse_y) {
    if (!btn) return false;
    return (mouse_x >= btn->x && mouse_x < btn->x + btn->width &&
            mouse_y >= btn->y && mouse_y < btn->y + btn->height);
}

void vga_button_click(vga_button_t* btn) {
    if (!btn || !btn->enabled) return;
    if (btn->on_click) {
        btn->on_click();
    }
}

// ===== PROGRESS BAR FUNCTIONS =====

vga_progress_bar_t vga_create_progress_bar(int x, int y, int width) {
    vga_progress_bar_t bar;
    bar.x = x;
    bar.y = y;
    bar.width = width;
    bar.progress = 0;
    bar.fg = VGA_COLOR_LIGHT_GREEN;
    bar.bg = VGA_COLOR_DARK_GREY;
    bar.show_percentage = true;
    return bar;
}

void vga_progress_bar_draw(vga_window_t* win, vga_progress_bar_t* bar) {
    if (!win || !bar) return;
    
    int filled = (bar->progress * bar->width) / 100;
    if (filled > bar->width) filled = bar->width;
    
    // Draw filled portion
    uint8_t fill_color = vga_entry_color(bar->fg, bar->fg);
    for (int i = 0; i < filled; i++) {
        vga_win_putc_colored(win, bar->x + i, bar->y, 0xDB, fill_color);
    }
    
    // Draw empty portion
    uint8_t empty_color = vga_entry_color(bar->bg, bar->bg);
    for (int i = filled; i < bar->width; i++) {
        vga_win_putc_colored(win, bar->x + i, bar->y, 0xB0, empty_color);
    }
    
    // Draw percentage text
    if (bar->show_percentage) {
        char percent_str[8];
        itoa(bar->progress, percent_str, 10);
        strcat(percent_str, "%");
        
        int text_len = strlen(percent_str);
        int text_x = bar->x + (bar->width - text_len) / 2;
        
        uint8_t text_color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
        for (int i = 0; i < text_len; i++) {
            vga_win_putc_colored(win, text_x + i, bar->y, percent_str[i], text_color);
        }
    }
}

void vga_progress_bar_set(vga_progress_bar_t* bar, int progress) {
    if (!bar) return;
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;
    bar->progress = progress;
}

void vga_progress_bar_increment(vga_progress_bar_t* bar, int amount) {
    if (!bar) return;
    bar->progress += amount;
    if (bar->progress > 100) bar->progress = 100;
}

// ===== TEXTBOX FUNCTIONS =====

vga_textbox_t vga_create_textbox(int x, int y, int w, int h, bool editable) {
    vga_textbox_t box;
    box.x = x;
    box.y = y;
    box.width = w;
    box.height = h;
    box.scroll_offset = 0;
    box.cursor_pos = 0;
    box.editable = editable;
    box.text = (char*)malloc(1024);
    if (box.text) {
        box.text[0] = '\0';
    }
    return box;
}

void vga_textbox_draw(vga_window_t* win, vga_textbox_t* box) {
    if (!win || !box || !box->text) return;
    
    // Draw border
    vga_win_draw_box(win, box->x, box->y, box->width, box->height);
    
    // Draw text content
    int line = 0;
    int col = 0;
    int i = box->scroll_offset;
    
    while (box->text[i] && line < box->height - 2) {
        if (box->text[i] == '\n') {
            line++;
            col = 0;
        } else {
            if (col < box->width - 2) {
                vga_win_putc(win, box->x + 1 + col, box->y + 1 + line, box->text[i]);
                col++;
            } else {
                line++;
                col = 0;
            }
        }
        i++;
    }
    
    // Draw cursor if editable
    if (box->editable) {
        int cursor_x = box->x + 1 + (box->cursor_pos % (box->width - 2));
        int cursor_y = box->y + 1 + (box->cursor_pos / (box->width - 2));
        if (cursor_y < box->y + box->height - 1) {
            vga_win_putc_colored(win, cursor_x, cursor_y, '_', 
                                vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        }
    }
}

void vga_textbox_set_text(vga_textbox_t* box, const char* text) {
    if (!box || !box->text || !text) return;
    strncpy(box->text, text, 1023);
    box->text[1023] = '\0';
    box->cursor_pos = strlen(box->text);
}

void vga_textbox_append(vga_textbox_t* box, const char* text) {
    if (!box || !box->text || !text) return;
    int current_len = strlen(box->text);
    int text_len = strlen(text);
    if (current_len + text_len < 1024) {
        strcat(box->text, text);
        box->cursor_pos = strlen(box->text);
    }
}

void vga_textbox_clear(vga_textbox_t* box) {
    if (!box || !box->text) return;
    box->text[0] = '\0';
    box->cursor_pos = 0;
    box->scroll_offset = 0;
}

void vga_textbox_scroll_up(vga_textbox_t* box) {
    if (!box) return;
    if (box->scroll_offset > 0) {
        box->scroll_offset--;
    }
}

void vga_textbox_scroll_down(vga_textbox_t* box) {
    if (!box || !box->text) return;
    int len = strlen(box->text);
    if (box->scroll_offset < len - 1) {
        box->scroll_offset++;
    }
}

// ===== MENU FUNCTIONS =====

vga_menu_t vga_create_menu(int x, int y, int width, const char** items, int count) {
    vga_menu_t menu;
    menu.x = x;
    menu.y = y;
    menu.width = width;
    menu.height = count + 2;
    menu.item_count = count;
    menu.selected_index = 0;
    
    menu.items = (char**)malloc(sizeof(char*) * count);
    for (int i = 0; i < count; i++) {
        menu.items[i] = (char*)malloc(64);
        strncpy(menu.items[i], items[i], 63);
        menu.items[i][63] = '\0';
    }
    
    return menu;
}

void vga_menu_draw(vga_window_t* win, vga_menu_t* menu) {
    if (!win || !menu) return;
    
    // Draw border
    vga_win_draw_box(win, menu->x, menu->y, menu->width, menu->height);
    
    // Draw items
    for (int i = 0; i < menu->item_count; i++) {
        uint8_t color;
        char prefix[4] = "  ";
        
        if (i == menu->selected_index) {
            color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
            strcpy(prefix, "> ");
        } else {
            color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
        
        // Draw prefix
        vga_win_puts_colored(win, menu->x + 1, menu->y + 1 + i, prefix, color);
        
        // Draw item text
        vga_win_puts_colored(win, menu->x + 3, menu->y + 1 + i, menu->items[i], color);
    }
}

void vga_menu_select_next(vga_menu_t* menu) {
    if (!menu) return;
    menu->selected_index++;
    if (menu->selected_index >= menu->item_count) {
        menu->selected_index = 0;
    }
}

void vga_menu_select_prev(vga_menu_t* menu) {
    if (!menu) return;
    menu->selected_index--;
    if (menu->selected_index < 0) {
        menu->selected_index = menu->item_count - 1;
    }
}

int vga_menu_get_selected(vga_menu_t* menu) {
    if (!menu) return -1;
    return menu->selected_index;
}


vga_sprite_t vga_create_sprite(int w, int h) {
    vga_sprite_t sprite;
    sprite.width = w;
    sprite.height = h;
    sprite.data = (uint16_t*)malloc(sizeof(uint16_t) * w * h);
    
    if (sprite.data) {
        uint16_t transparent = vga_entry(0, vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_BLACK));
        for (int i = 0; i < w * h; i++) {
            sprite.data[i] = transparent;
        }
    }
    
    return sprite;
}

void vga_sprite_destroy(vga_sprite_t* sprite) {
    if (!sprite) return;
    if (sprite->data) {
        free(sprite->data);
        sprite->data = NULL;
    }
}

void vga_sprite_set_pixel(vga_sprite_t* sprite, int x, int y, char ch, uint8_t color) {
    if (!sprite || !sprite->data) return;
    if (x < 0 || y < 0 || x >= sprite->width || y >= sprite->height) return;
    sprite->data[y * sprite->width + x] = vga_entry(ch, color);
}

void vga_sprite_draw(vga_window_t* win, vga_sprite_t* sprite, int x, int y) {
    if (!win || !sprite || !sprite->data) return;
    
    for (int row = 0; row < sprite->height; row++) {
        for (int col = 0; col < sprite->width; col++) {
            int wx = x + col;
            int wy = y + row;
            if (wx >= 0 && wx < win->width && wy >= 0 && wy < win->height) {
                win->buffer[wy * win->width + wx] = sprite->data[row * sprite->width + col];
            }
        }
    }
}

void vga_sprite_draw_transparent(vga_window_t* win, vga_sprite_t* sprite, int x, int y, uint16_t transparent) {
    if (!win || !sprite || !sprite->data) return;
    
    for (int row = 0; row < sprite->height; row++) {
        for (int col = 0; col < sprite->width; col++) {
            uint16_t pixel = sprite->data[row * sprite->width + col];
            if (pixel != transparent) {
                int wx = x + col;
                int wy = y + row;
                if (wx >= 0 && wx < win->width && wy >= 0 && wy < win->height) {
                    win->buffer[wy * win->width + wx] = pixel;
                }
            }
        }
    }
}

// ===== ANIMATION FUNCTIONS =====

vga_animation_t vga_create_animation(int frame_count, int frame_delay) {
    vga_animation_t anim;
    anim.frame_count = frame_count;
    anim.frame_delay = frame_delay;
    anim.current_frame = 0;
    anim.tick_counter = 0;
    anim.loop = true;
    anim.playing = false;
    
    anim.frames = (vga_sprite_t*)malloc(sizeof(vga_sprite_t) * frame_count);
    
    return anim;
}

void vga_animation_destroy(vga_animation_t* anim) {
    if (!anim) return;
    if (anim->frames) {
        for (int i = 0; i < anim->frame_count; i++) {
            vga_sprite_destroy(&anim->frames[i]);
        }
        free(anim->frames);
        anim->frames = NULL;
    }
}

void vga_animation_add_frame(vga_animation_t* anim, vga_sprite_t* frame, int index) {
    if (!anim || !frame || index < 0 || index >= anim->frame_count) return;
    anim->frames[index] = *frame;
}

void vga_animation_play(vga_animation_t* anim) {
    if (!anim) return;
    anim->playing = true;
}

void vga_animation_stop(vga_animation_t* anim) {
    if (!anim) return;
    anim->playing = false;
}

void vga_animation_reset(vga_animation_t* anim) {
    if (!anim) return;
    anim->current_frame = 0;
    anim->tick_counter = 0;
}

void vga_animation_update(vga_animation_t* anim) {
    if (!anim || !anim->playing) return;
    
    anim->tick_counter++;
    if (anim->tick_counter >= anim->frame_delay) {
        anim->tick_counter = 0;
        anim->current_frame++;
        
        if (anim->current_frame >= anim->frame_count) {
            if (anim->loop) {
                anim->current_frame = 0;
            } else {
                anim->current_frame = anim->frame_count - 1;
                anim->playing = false;
            }
        }
    }
}

void vga_animation_draw(vga_window_t* win, vga_animation_t* anim, int x, int y) {
    if (!anim || !anim->frames) return;
    if (anim->current_frame < 0 || anim->current_frame >= anim->frame_count) return;
    
    vga_sprite_draw(win, &anim->frames[anim->current_frame], x, y);
}

// ===== PARTICLE SYSTEM =====

vga_particle_system_t* vga_particle_system_create(int max_particles) {
    vga_particle_system_t* system = (vga_particle_system_t*)malloc(sizeof(vga_particle_system_t));
    if (!system) return NULL;
    
    system->max_particles = max_particles;
    system->active_particles = 0;
    system->particles = (vga_particle_t*)malloc(sizeof(vga_particle_t) * max_particles);
    
    if (!system->particles) {
        free(system);
        return NULL;
    }
    
    return system;
}

void vga_particle_system_destroy(vga_particle_system_t* system) {
    if (!system) return;
    if (system->particles) {
        free(system->particles);
    }
    free(system);
}

void vga_particle_system_emit(vga_particle_system_t* system, float x, float y, 
                               float vx, float vy, int life, char ch, uint8_t color) {
    if (!system || !system->particles) return;
    if (system->active_particles >= system->max_particles) return;
    
    vga_particle_t* p = &system->particles[system->active_particles];
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->life = life;
    p->character = ch;
    p->color = color;
    
    system->active_particles++;
}

void vga_particle_system_update(vga_particle_system_t* system) {
    if (!system || !system->particles) return;
    
    for (int i = 0; i < system->active_particles; i++) {
        vga_particle_t* p = &system->particles[i];
        
        // Update position
        p->x += p->vx;
        p->y += p->vy;
        
        // Apply gravity
        p->vy += 0.1f;
        
        // Decrease life
        p->life--;
        
        // Remove dead particles
        if (p->life <= 0) {
            // Swap with last particle
            system->particles[i] = system->particles[system->active_particles - 1];
            system->active_particles--;
            i--;
        }
    }
}

void vga_particle_system_draw(vga_window_t* win, vga_particle_system_t* system) {
    if (!win || !system || !system->particles) return;
    
    for (int i = 0; i < system->active_particles; i++) {
        vga_particle_t* p = &system->particles[i];
        
        int x = (int)p->x;
        int y = (int)p->y;
        
        if (x >= 0 && x < win->width && y >= 0 && y < win->height) {
            vga_win_putc_colored(win, x, y, p->character, p->color);
        }
    }
}

// ===== SPECIAL EFFECTS =====

void vga_win_fade_in(vga_window_t* win, int steps) {
    if (!win) return;
    
    for (int step = 0; step < steps; step++) {
        // Gradually increase visibility (simplified)
        vga_win_refresh(win);
        delay(50);
    }
}

void vga_win_fade_out(vga_window_t* win, int steps) {
    if (!win) return;
    
    for (int step = steps; step > 0; step--) {
        // Gradually decrease visibility (simplified)
        delay(50);
    }
    
    vga_destroy_window(win);
}

void vga_win_shake(vga_window_t* win, int intensity, int duration) {
    if (!win) return;
    
    int original_x = win->x;
    int original_y = win->y;
    
    for (int i = 0; i < duration; i++) {
        int offset_x = (i % 2 == 0) ? intensity : -intensity;
        int offset_y = (i % 3 == 0) ? intensity : -intensity;
        
        vga_win_move(win, original_x + offset_x, original_y + offset_y);
        delay(20);
    }
    
    // Return to original position
    vga_win_move(win, original_x, original_y);
}

void vga_win_flash(vga_window_t* win, uint8_t color, int times) {
    if (!win) return;
    
    uint8_t original_color = win->color;
    
    for (int i = 0; i < times; i++) {
        win->color = color;
        vga_win_refresh(win);
        delay(100);
        
        win->color = original_color;
        vga_win_refresh(win);
        delay(100);
    }
}

void vga_win_matrix_rain(vga_window_t* win, int duration) {
    if (!win) return;
    
    uint8_t green = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    
    for (int frame = 0; frame < duration; frame++) {
        // Clear window
        vga_win_clear(win);
        
        // Draw random characters falling
        for (int col = 0; col < win->width; col++) {
            if ((frame + col) % 5 == 0) {
                int start_row = (frame + col * 7) % (win->height + 10) - 10;
                for (int i = 0; i < 5; i++) {
                    int row = start_row + i;
                    if (row >= 0 && row < win->height) {
                        char ch = '0' + ((frame + col + i) % 10);
                        vga_win_putc_colored(win, col, row, ch, green);
                    }
                }
            }
        }
        
        vga_win_refresh(win);
        delay(50);
    }
}

void vga_win_starfield(vga_window_t* win, int star_count, int duration) {
    if (!win) return;
    
    // Simple starfield effect
    for (int frame = 0; frame < duration; frame++) {
        vga_win_clear(win);
        
        for (int i = 0; i < star_count; i++) {
            int x = (frame * i * 7) % win->width;
            int y = (i * 13) % win->height;
            
            char star_char = (i % 3 == 0) ? '*' : '.';
            uint8_t color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            
            vga_win_putc_colored(win, x, y, star_char, color);
        }
        
        vga_win_refresh(win);
        delay(30);
    }
}

// ===== ASCII ART =====

void vga_win_draw_ascii_art(vga_window_t* win, int x, int y, const char* art[], int lines) {
    if (!win || !art) return;
    
    for (int i = 0; i < lines; i++) {
        vga_win_puts(win, x, y + i, art[i]);
    }
}

void vga_win_draw_logo(vga_window_t* win) {
    if (!win) return;
    
    const char* logo[] = {
        " ____            _ _  ___  ____  ",
        "|  _ \\ __ _  __| (_)/ _ \\/ ___| ",
        "| |_) / _` |/ _` | | | | \\___ \\ ",
        "|  _ < (_| | (_| | | |_| |___) |",
        "|_| \\_\\__,_|\\__,_|_|\\___/|____/ "
    };
    
    int start_y = (win->height - 5) / 2;
    vga_win_draw_ascii_art(win, 2, start_y, logo, 5);
}

void vga_win_draw_loading_spinner(vga_window_t* win, int x, int y, int frame) {
    const char spinners[] = {'|', '/', '-', '\\'};
    char spinner = spinners[frame % 4];
    
    vga_win_putc(win, x, y, spinner);
}