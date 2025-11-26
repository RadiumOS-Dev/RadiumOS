// vga.h - Advanced VGA Graphics Library
#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stdbool.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// VGA Colors
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Window structure
typedef struct {
    int x, y;              // Position on screen
    int width, height;     // Dimensions
    uint8_t color;         // Default color
    uint16_t* buffer;      // Window buffer
    bool visible;          // Visibility flag
    bool has_border;       // Draw border
    bool has_shadow;       // Draw shadow
    char title[64];        // Window title
} vga_window_t;

// Button structure
typedef struct {
    int x, y;              // Position in window
    int width, height;     // Dimensions
    char text[32];         // Button text
    bool enabled;          // Can be clicked
    bool selected;         // Currently selected
    void (*on_click)(void);// Callback function
} vga_button_t;

// Progress bar structure
typedef struct {
    int x, y;              // Position in window
    int width;             // Width in chars
    int progress;          // 0-100
    enum vga_color fg;     // Fill color
    enum vga_color bg;     // Background color
    bool show_percentage;  // Show % text
} vga_progress_bar_t;

// Text box structure
typedef struct {
    int x, y;              // Position in window
    int width, height;     // Dimensions
    char* text;            // Text content
    int scroll_offset;     // For scrolling
    int cursor_pos;        // Cursor position
    bool editable;         // Can edit text
} vga_textbox_t;

// Menu structure
typedef struct {
    int x, y;              // Position in window
    int width, height;     // Dimensions
    char** items;          // Menu items
    int item_count;        // Number of items
    int selected_index;    // Current selection
} vga_menu_t;

// Sprite structure (for simple graphics)
typedef struct {
    int width, height;     // Dimensions
    uint16_t* data;        // Sprite data
} vga_sprite_t;

// Animation structure
typedef struct {
    vga_sprite_t* frames;  // Array of frames
    int frame_count;       // Number of frames
    int current_frame;     // Current frame index
    int frame_delay;       // Ticks per frame
    int tick_counter;      // Current tick
    bool loop;             // Loop animation
    bool playing;          // Is playing
} vga_animation_t;

// Particle structure (for effects)
typedef struct {
    float x, y;            // Position
    float vx, vy;          // Velocity
    int life;              // Lifetime remaining
    char character;        // Display character
    uint8_t color;         // Color
} vga_particle_t;

// Particle system
typedef struct {
    vga_particle_t* particles;
    int max_particles;
    int active_particles;
} vga_particle_system_t;

// ===== CORE VGA FUNCTIONS =====
uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg);
uint16_t vga_entry(unsigned char uc, uint8_t color);

// ===== WINDOW MANAGEMENT =====
vga_window_t vga_create_window(int x, int y, int w, int h, enum vga_color fg, enum vga_color bg);
vga_window_t vga_create_centered_window(int w, int h, enum vga_color fg, enum vga_color bg);
void vga_destroy_window(vga_window_t* win);
void vga_win_clear(vga_window_t* win);
void vga_win_refresh(vga_window_t* win);
void vga_win_set_title(vga_window_t* win, const char* title);
void vga_win_show(vga_window_t* win);
void vga_win_hide(vga_window_t* win);
void vga_win_move(vga_window_t* win, int new_x, int new_y);
void vga_win_resize(vga_window_t* win, int new_w, int new_h);

// ===== DRAWING FUNCTIONS =====
void vga_win_putc(vga_window_t* win, int wx, int wy, char c);
void vga_win_putc_colored(vga_window_t* win, int wx, int wy, char c, uint8_t color);
void vga_win_puts(vga_window_t* win, int wx, int wy, const char* str);
void vga_win_puts_colored(vga_window_t* win, int wx, int wy, const char* str, uint8_t color);
void vga_win_puts_centered(vga_window_t* win, int y, const char* str);
void vga_win_puts_centered_offset(vga_window_t* win, const char* str, int y_offset);
void vga_win_draw_box(vga_window_t* win, int x, int y, int w, int h);
void vga_win_draw_box_colored(vga_window_t* win, int x, int y, int w, int h, uint8_t color);
void vga_win_fill_rect(vga_window_t* win, int x, int y, int w, int h, char fill_char, uint8_t color);
void vga_win_draw_line_h(vga_window_t* win, int x, int y, int length, char ch);
void vga_win_draw_line_v(vga_window_t* win, int x, int y, int length, char ch);
void vga_win_draw_shadow(vga_window_t* win, int x, int y, int w, int h);

// ===== BUTTON FUNCTIONS =====
vga_button_t vga_create_button(int x, int y, int w, int h, const char* text);
void vga_win_draw_button(vga_window_t* win, int x, int y, int w, int h, const char* text, bool selected, int frame_counter);
void vga_button_draw(vga_window_t* win, vga_button_t* btn, int frame_counter);
void vga_button_set_callback(vga_button_t* btn, void (*callback)(void));
bool vga_button_is_hovered(vga_button_t* btn, int mouse_x, int mouse_y);
void vga_button_click(vga_button_t* btn);

// ===== PROGRESS BAR FUNCTIONS =====
vga_progress_bar_t vga_create_progress_bar(int x, int y, int width);
void vga_progress_bar_draw(vga_window_t* win, vga_progress_bar_t* bar);
void vga_progress_bar_set(vga_progress_bar_t* bar, int progress);
void vga_progress_bar_increment(vga_progress_bar_t* bar, int amount);

// ===== TEXTBOX FUNCTIONS =====
vga_textbox_t vga_create_textbox(int x, int y, int w, int h, bool editable);
void vga_textbox_draw(vga_window_t* win, vga_textbox_t* box);
void vga_textbox_set_text(vga_textbox_t* box, const char* text);
void vga_textbox_append(vga_textbox_t* box, const char* text);
void vga_textbox_clear(vga_textbox_t* box);
void vga_textbox_scroll_up(vga_textbox_t* box);
void vga_textbox_scroll_down(vga_textbox_t* box);

// ===== MENU FUNCTIONS =====
vga_menu_t vga_create_menu(int x, int y, int width, const char** items, int count);
void vga_menu_draw(vga_window_t* win, vga_menu_t* menu);
void vga_menu_select_next(vga_menu_t* menu);
void vga_menu_select_prev(vga_menu_t* menu);
int vga_menu_get_selected(vga_menu_t* menu);

// ===== SPRITE FUNCTIONS =====
vga_sprite_t vga_create_sprite(int w, int h);
void vga_sprite_destroy(vga_sprite_t* sprite);
void vga_sprite_set_pixel(vga_sprite_t* sprite, int x, int y, char ch, uint8_t color);
void vga_sprite_draw(vga_window_t* win, vga_sprite_t* sprite, int x, int y);
void vga_sprite_draw_transparent(vga_window_t* win, vga_sprite_t* sprite, int x, int y, uint16_t transparent);

// ===== ANIMATION FUNCTIONS =====
vga_animation_t vga_create_animation(int frame_count, int frame_delay);
void vga_animation_destroy(vga_animation_t* anim);
void vga_animation_add_frame(vga_animation_t* anim, vga_sprite_t* frame, int index);
void vga_animation_play(vga_animation_t* anim);
void vga_animation_stop(vga_animation_t* anim);
void vga_animation_reset(vga_animation_t* anim);
void vga_animation_update(vga_animation_t* anim);
void vga_animation_draw(vga_window_t* win, vga_animation_t* anim, int x, int y);

// ===== PARTICLE SYSTEM =====
vga_particle_system_t* vga_particle_system_create(int max_particles);
void vga_particle_system_destroy(vga_particle_system_t* system);
void vga_particle_system_emit(vga_particle_system_t* system, float x, float y, float vx, float vy, int life, char ch, uint8_t color);
void vga_particle_system_update(vga_particle_system_t* system);
void vga_particle_system_draw(vga_window_t* win, vga_particle_system_t* system);

// ===== SPECIAL EFFECTS =====
void vga_win_fade_in(vga_window_t* win, int steps);
void vga_win_fade_out(vga_window_t* win, int steps);
void vga_win_shake(vga_window_t* win, int intensity, int duration);
void vga_win_flash(vga_window_t* win, uint8_t color, int times);
void vga_win_matrix_rain(vga_window_t* win, int duration);
void vga_win_starfield(vga_window_t* win, int star_count, int duration);

// ===== ASCII ART =====
void vga_win_draw_ascii_art(vga_window_t* win, int x, int y, const char* art[], int lines);
void vga_win_draw_logo(vga_window_t* win);
void vga_win_draw_loading_spinner(vga_window_t* win, int x, int y, int frame);

// ===== DIALOG FUNCTIONS =====
bool vga_dialog_confirm(const char* title, const char* message);
void vga_dialog_alert(const char* title, const char* message);
int vga_dialog_choice(const char* title, const char* message, const char** choices, int count);
void vga_dialog_input(const char* title, const char* prompt, char* buffer, int max_len);

// ===== UTILITY FUNCTIONS =====
void vga_show_hello_world(void);
void vga_demo_all_features(void);
void vga_test_colors(void);
void vga_test_animations(void);
void vga_benchmark(void);

#endif // VGA_H