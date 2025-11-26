#ifndef TILINGMANAGER_H
#define TILINGMANAGER_H

#include "vga.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Menu item flags
#define MENU_ITEM_DISABLED   (1 << 0)
#define MENU_ITEM_SEPARATOR  (1 << 1)
#define MENU_ITEM_SUBMENU    (1 << 2)
#define MENU_ITEM_CHECKBOX   (1 << 3)
#define MENU_ITEM_RADIO      (1 << 4)

// Theme IDs
#define MENU_THEME_DEFAULT   0
#define MENU_THEME_MATRIX    1
#define MENU_THEME_RETRO     2
#define MENU_THEME_FIRE      3

// Configuration
#define MAX_MENU_ITEMS 50
#define MAX_TITLE_LENGTH 64
#define MAX_TOOLTIP_LENGTH 128

// Function pointer types
typedef void (*MenuFunction)(void);
typedef void (*MenuCallback)(int item_index, void* user_data);

// Forward declaration for multi_panel_menu_t (if needed for compatibility)
typedef struct multi_panel_menu_t multi_panel_menu_t;

// Color theme structure - DEFINE HERE, NOT FORWARD DECLARE
typedef struct {
    enum vga_color fg_normal;
    enum vga_color bg_normal;
    enum vga_color fg_selected;
    enum vga_color bg_selected;
    enum vga_color fg_disabled;
    enum vga_color border_color;
    enum vga_color title_color;
} MenuTheme;

// Menu item structure - DEFINE HERE
typedef struct MenuItemEx {
    const char* name;
    const char* tooltip;
    const char* shortcut_key;
    MenuFunction action;
    MenuCallback callback;
    void* user_data;
    enum vga_color color;
    uint8_t flags;
    bool checked;
    int radio_group;
    struct MenuItemEx* submenu;
    int submenu_count;
} MenuItemEx;

// Menu state structure - DEFINE HERE, NOT FORWARD DECLARE
typedef struct {
    MenuItemEx items[MAX_MENU_ITEMS];
    int item_count;
    int selected_index;
    int scroll_offset;
    int visible_items;
    MenuTheme theme;
    char title[MAX_TITLE_LENGTH];
    bool show_tooltips;
    bool show_shortcuts;
    bool animate_selection;
    int animation_frame;
} MenuState;

// API Functions
void vga_menu_init(void);
void vga_menu_set_title(const char* title);
void vga_menu_set_theme(int theme_id);

void vga_menu_add(const char* name, MenuFunction action, enum vga_color color);
void vga_menu_add_ex(const char* name, const char* tooltip, const char* shortcut,
                     MenuFunction action, enum vga_color color, uint8_t flags);
void vga_menu_add_separator(void);
void vga_menu_add_checkbox(const char* name, const char* tooltip, bool initial_state);
void vga_menu_toggle_checkbox(int index);
void vga_menu_remove(int index);

void vga_menu_render(multi_panel_menu_t* menu);
void vga_menu_render_ex(MenuState* state);
void vga_menu_run(int argc, char* argv[]);
void vga_menu_destroy(multi_panel_menu_t* menu);

void vga_menu_progress_bar(const char* title, int delay, char symbol);
void vga_menu_progress_bar_ex(const char* title, const char* subtitle, 
                               int total_steps, int delay, char symbol,
                               enum vga_color bar_color);
void vga_menu_show_dialog(const char* title, const char* message, enum vga_color color);

void vga_menu_example_progress(void);
void vga_menu_example_themes(void);

vga_window_t vga_create_window_static(int x, int y, unsigned int width, unsigned int height, 
                                       enum vga_color fg, enum vga_color bg, 
                                       uint16_t* static_buffer, size_t buffer_size);

#endif // TILINGMANAGER_H