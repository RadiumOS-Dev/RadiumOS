#include "vga.h"                    // For vga_window_t, enum vga_color, VGA_WIDTH, VGA_HEIGHT
#include "../utility/utility.h"     // For strlen, itoa
#include "../keyboard/keyboard.h"   // For keyboard_key()
#include "../timers/timer.h"        // If needed for delays
#include <stddef.h>                 // For NULL, size_t
#include <stdint.h>                 // For uint types
#include <stdbool.h>                // For bool
#include "tilingmanager.h"
// Fallback for bool in freestanding env (if <stdbool.h> not supported)
#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

// Menu configuration - Enhanced limits
#define MAX_MENU_ITEMS 50
#define MAX_SUBMENU_DEPTH 5
#define MAX_WINDOW_WIDTH 80
#define MAX_WINDOW_HEIGHT 25
#define MAX_TOOLTIP_LENGTH 128
#define MAX_TITLE_LENGTH 64

// Animation configuration
#define ANIMATION_FRAME_DELAY 50000
#define MENU_TRANSITION_FRAMES 10

// Enhanced menu item flags
#define MENU_ITEM_DISABLED   (1 << 0)
#define MENU_ITEM_SEPARATOR  (1 << 1)
#define MENU_ITEM_SUBMENU    (1 << 2)
#define MENU_ITEM_CHECKBOX   (1 << 3)
#define MENU_ITEM_RADIO      (1 << 4)


// NOTE: MenuTheme, MenuItemEx, MenuState, MenuFunction, MenuCallback
// are now defined in vga_menu_enhanced.h - don't duplicate them here!

// Menu stack for submenu navigation
typedef struct {
    MenuState* states[MAX_SUBMENU_DEPTH];
    int depth;
} MenuStack;

// Global, static buffers for the windows
static uint16_t menu_buffer[MAX_WINDOW_WIDTH * MAX_WINDOW_HEIGHT];
static uint16_t progress_bar_buffer[MAX_WINDOW_WIDTH * MAX_WINDOW_HEIGHT];
static uint16_t tooltip_buffer[MAX_WINDOW_WIDTH * 3]; // Smaller buffer for tooltip
static uint16_t dialog_buffer[MAX_WINDOW_WIDTH * 10]; // For dialogs

// Global menu state
static MenuState root_menu_state;
static MenuStack menu_stack;
static vga_window_t menu_win;
static int menu_win_valid = 0;

// Predefined themes
static MenuTheme theme_default = {
    VGA_COLOR_WHITE, VGA_COLOR_BLACK,      // Normal
    VGA_COLOR_BLACK, VGA_COLOR_WHITE,      // Selected
    VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK,  // Disabled
    VGA_COLOR_LIGHT_GREY,                  // Border
    VGA_COLOR_CYAN                         // Title
};

static MenuTheme theme_matrix = {
    VGA_COLOR_GREEN, VGA_COLOR_BLACK,      // Normal
    VGA_COLOR_BLACK, VGA_COLOR_GREEN,      // Selected
    VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK,  // Disabled
    VGA_COLOR_GREEN,                       // Border
    VGA_COLOR_LIGHT_GREEN                  // Title
};

static MenuTheme theme_retro = {
    VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLUE,      // Normal
    VGA_COLOR_BLUE, VGA_COLOR_LIGHT_BROWN,      // Selected
    VGA_COLOR_DARK_GREY, VGA_COLOR_BLUE,   // Disabled
    VGA_COLOR_LIGHT_CYAN,                  // Border
    VGA_COLOR_WHITE                        // Title
};

static MenuTheme theme_fire = {
    VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK,  // Normal
    VGA_COLOR_BLACK, VGA_COLOR_LIGHT_RED,  // Selected
    VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK,  // Disabled
    VGA_COLOR_RED,                         // Border
    VGA_COLOR_LIGHT_BROWN                       // Title
};

// Function to create a window using a pre-allocated static buffer.
vga_window_t vga_create_window_static(int x, int y, unsigned int width, unsigned int height, 
                                       enum vga_color fg, enum vga_color bg, 
                                       uint16_t* static_buffer, size_t buffer_size) {
    vga_window_t win;
    win.x = x;
    win.y = y;
    win.width = width;
    win.height = height;
    win.color = vga_entry_color(fg, bg);

    if (width * height * sizeof(uint16_t) <= buffer_size) {
        win.buffer = static_buffer;
    } else {
        win.buffer = NULL;
    }
    return win;
}

// Initialize menu state
static void init_menu_state(MenuState* state) {
    state->item_count = 0;
    state->selected_index = 0;
    state->scroll_offset = 0;
    state->visible_items = 0;
    state->theme = theme_default;
    state->title[0] = '\0';
    state->show_tooltips = true;
    state->show_shortcuts = true;
    state->animate_selection = true;
    state->animation_frame = 0;
}

// Initialize the menu system
void vga_menu_init(void) {
    init_menu_state(&root_menu_state);
    menu_stack.depth = 0;
    menu_stack.states[0] = &root_menu_state;
}

// Set menu theme
void vga_menu_set_theme(int theme_id) {
    MenuTheme* theme = &theme_default;
    switch (theme_id) {
        case 1: theme = &theme_matrix; break;
        case 2: theme = &theme_retro; break;
        case 3: theme = &theme_fire; break;
        default: theme = &theme_default; break;
    }
    root_menu_state.theme = *theme;
}

// Set menu title
void vga_menu_set_title(const char* title) {
    if (!title) return;
    int i = 0;
    while (title[i] && i < MAX_TITLE_LENGTH - 1) {
        root_menu_state.title[i] = title[i];
        i++;
    }
    root_menu_state.title[i] = '\0';
}

// Helper to create a menu window with proper sizing
static vga_window_t create_menu_window_ex(MenuState* state) {
    int num_items = state->item_count;
    
    // Calculate required width (find longest item)
    int max_width = strlen(state->title) + 4;
    for (int i = 0; i < num_items; i++) {
        int item_width = strlen(state->items[i].name);
        if (state->show_shortcuts && state->items[i].shortcut_key) {
            item_width += strlen(state->items[i].shortcut_key) + 3;
        }
        if (state->items[i].flags & MENU_ITEM_CHECKBOX) {
            item_width += 4; // For [X] or [ ]
        }
        if (state->items[i].flags & MENU_ITEM_SUBMENU) {
            item_width += 2; // For >
        }
        if (item_width > max_width) max_width = item_width;
    }
    
    int width = max_width + 6; // Padding and border
    if (width > VGA_WIDTH) width = VGA_WIDTH;
    if (width < 20) width = 20;
    
    // Calculate height
    int max_visible = VGA_HEIGHT - 10; // Leave room for other elements
    state->visible_items = (num_items < max_visible) ? num_items : max_visible;
    int height = state->visible_items + 4; // Items + border + title
    if (state->show_tooltips) height += 2; // Extra space for tooltip
    
    int x = (VGA_WIDTH - width) / 2;
    int y = (VGA_HEIGHT - height) / 2;

    return vga_create_window_static(x, y, width, height, 
                                    state->theme.fg_normal, 
                                    state->theme.bg_normal, 
                                    menu_buffer, sizeof(menu_buffer));
}

// Draw a fancy border with double lines
static void draw_fancy_border(vga_window_t* win, MenuState* state) {
    uint8_t border_color = vga_entry_color(state->theme.border_color, state->theme.bg_normal);
    
    // Corners and lines
    win->buffer[0] = vga_entry(201, border_color); // Top-left
    win->buffer[win->width - 1] = vga_entry(187, border_color); // Top-right
    win->buffer[(win->height - 1) * win->width] = vga_entry(200, border_color); // Bottom-left
    win->buffer[(win->height - 1) * win->width + win->width - 1] = vga_entry(188, border_color); // Bottom-right
    
    // Top and bottom horizontal lines
    for (unsigned int i = 1; i < win->width - 1; i++) {
        win->buffer[i] = vga_entry(205, border_color);
        win->buffer[(win->height - 1) * win->width + i] = vga_entry(205, border_color);
    }
    
    // Left and right vertical lines
    for (unsigned int i = 1; i < win->height - 1; i++) {
        win->buffer[i * win->width] = vga_entry(186, border_color);
        win->buffer[i * win->width + win->width - 1] = vga_entry(186, border_color);
    }
    
    // Title separator (if title exists)
    if (state->title[0] != '\0') {
        win->buffer[2 * win->width] = vga_entry(204, border_color); // Left T
        win->buffer[2 * win->width + win->width - 1] = vga_entry(185, border_color); // Right T
        for (unsigned int i = 1; i < win->width - 1; i++) {
            win->buffer[2 * win->width + i] = vga_entry(205, border_color);
        }
    }
}

// Render the enhanced menu
void vga_menu_render_ex(MenuState* state) {
    if (!menu_win_valid || !menu_win.buffer || state->item_count == 0) return;

    vga_win_clear(&menu_win);
    draw_fancy_border(&menu_win, state);
    
    // Draw title
    if (state->title[0] != '\0') {
        uint8_t title_color = vga_entry_color(state->theme.title_color, state->theme.bg_normal);
        int title_x = (menu_win.width - strlen(state->title)) / 2;
        if (title_x < 2) title_x = 2;
        for (int i = 0; state->title[i] && (title_x + i) < (int)menu_win.width - 2; i++) {
            menu_win.buffer[menu_win.width + title_x + i] = vga_entry(state->title[i], title_color);
        }
    }
    
    // Calculate starting row for items
    int start_row = (state->title[0] != '\0') ? 3 : 1;
    
    // Draw scroll indicator if needed
    if (state->scroll_offset > 0) {
        uint8_t scroll_color = vga_entry_color(state->theme.border_color, state->theme.bg_normal);
        int indicator_x = menu_win.width - 2;
        menu_win.buffer[start_row * menu_win.width + indicator_x] = vga_entry(24, scroll_color); // Up arrow
    }
    
    // Render visible menu items
    for (int i = 0; i < state->visible_items; i++) {
        int item_idx = state->scroll_offset + i;
        if (item_idx >= state->item_count) break;
        
        MenuItemEx* item = &state->items[item_idx];
        int row = start_row + i;
        
        // Handle separator
        if (item->flags & MENU_ITEM_SEPARATOR) {
            uint8_t border_color = vga_entry_color(state->theme.border_color, state->theme.bg_normal);
            menu_win.buffer[row * menu_win.width] = vga_entry(204, border_color);
            menu_win.buffer[row * menu_win.width + menu_win.width - 1] = vga_entry(185, border_color);
            for (unsigned int j = 1; j < menu_win.width - 1; j++) {
                menu_win.buffer[row * menu_win.width + j] = vga_entry(196, border_color);
            }
            continue;
        }
        
        // Determine colors
        bool is_selected = (item_idx == state->selected_index);
        bool is_disabled = (item->flags & MENU_ITEM_DISABLED);
        
        enum vga_color fg, bg;
        if (is_disabled) {
            fg = state->theme.fg_disabled;
            bg = state->theme.bg_normal;
        } else if (is_selected) {
            fg = state->theme.fg_selected;
            bg = state->theme.bg_selected;
            
            // Animation effect
            if (state->animate_selection && state->animation_frame % 4 < 2) {
                fg = state->theme.bg_selected;
                bg = state->theme.fg_selected;
            }
        } else {
            fg = item->color;
            bg = state->theme.bg_normal;
        }
        
        uint8_t item_color = vga_entry_color(fg, bg);
        
        int col = 2;
        
        // Draw checkbox/radio indicator
        if (item->flags & (MENU_ITEM_CHECKBOX | MENU_ITEM_RADIO)) {
            char checkbox_chars[4];
            if (item->flags & MENU_ITEM_CHECKBOX) {
                checkbox_chars[0] = '[';
                checkbox_chars[1] = item->checked ? 'X' : ' ';
                checkbox_chars[2] = ']';
                checkbox_chars[3] = ' ';
            } else {
                checkbox_chars[0] = '(';
                checkbox_chars[1] = item->checked ? '*' : ' ';
                checkbox_chars[2] = ')';
                checkbox_chars[3] = ' ';
            }
            for (int j = 0; j < 4; j++) {
                menu_win.buffer[row * menu_win.width + col++] = vga_entry(checkbox_chars[j], item_color);
            }
        }
        
        // Draw item name
        const char* name = item->name;
        while (*name && col < (int)menu_win.width - 2) {
            menu_win.buffer[row * menu_win.width + col++] = vga_entry(*name++, item_color);
        }
        
        // Fill remaining space
        while (col < (int)menu_win.width - 6) {
            menu_win.buffer[row * menu_win.width + col++] = vga_entry(' ', item_color);
        }
        
        // Draw shortcut key
        if (state->show_shortcuts && item->shortcut_key) {
            int shortcut_len = strlen(item->shortcut_key);
            int shortcut_x = menu_win.width - shortcut_len - 3;
            for (int j = 0; j < shortcut_len; j++) {
                menu_win.buffer[row * menu_win.width + shortcut_x + j] = 
                    vga_entry(item->shortcut_key[j], item_color);
            }
            col = shortcut_x + shortcut_len;
        }
        
        // Draw submenu indicator
        if (item->flags & MENU_ITEM_SUBMENU) {
            menu_win.buffer[row * menu_win.width + menu_win.width - 2] = vga_entry('>', item_color);
        }
    }
    
    // Draw scroll indicator if needed
    if (state->scroll_offset + state->visible_items < state->item_count) {
        uint8_t scroll_color = vga_entry_color(state->theme.border_color, state->theme.bg_normal);
        int indicator_x = menu_win.width - 2;
        int indicator_row = start_row + state->visible_items - 1;
        menu_win.buffer[indicator_row * menu_win.width + indicator_x] = vga_entry(25, scroll_color); // Down arrow
    }
    
    // Draw tooltip for selected item
    if (state->show_tooltips && state->selected_index >= 0 && 
        state->selected_index < state->item_count) {
        MenuItemEx* selected = &state->items[state->selected_index];
        if (selected->tooltip) {
            int tooltip_row = menu_win.height - 2;
            uint8_t tooltip_color = vga_entry_color(VGA_COLOR_LIGHT_BROWN, state->theme.bg_normal);
            int col = 2;
            const char* tip = selected->tooltip;
            while (*tip && col < (int)menu_win.width - 2) {
                menu_win.buffer[tooltip_row * menu_win.width + col++] = 
                    vga_entry(*tip++, tooltip_color);
            }
        }
    }
    
    vga_win_refresh(&menu_win);
}

// Add an enhanced menu item
void vga_menu_add_ex(const char* name, const char* tooltip, const char* shortcut,
                     MenuFunction action, enum vga_color color, uint8_t flags) {
    if (root_menu_state.item_count >= MAX_MENU_ITEMS || !name) return;
    
    MenuItemEx* item = &root_menu_state.items[root_menu_state.item_count];
    item->name = name;
    item->tooltip = tooltip;
    item->shortcut_key = shortcut;
    item->action = action;
    item->callback = NULL;
    item->user_data = NULL;
    item->color = color;
    item->flags = flags;
    item->checked = false;
    item->radio_group = 0;
    item->submenu = NULL;
    item->submenu_count = 0;
    
    root_menu_state.item_count++;
    
    // Recreate window if needed
    if (menu_win_valid) {
        vga_destroy_window(&menu_win);
    }
    menu_win = create_menu_window_ex(&root_menu_state);
    if (menu_win.buffer) {
        menu_win_valid = 1;
        vga_menu_render_ex(&root_menu_state);
    } else {
        menu_win_valid = 0;
    }
}

// Add a simple menu item (backward compatibility)
void vga_menu_add(const char* name, MenuFunction action, enum vga_color color) {
    vga_menu_add_ex(name, NULL, NULL, action, color, 0);
}

// Add a separator
void vga_menu_add_separator(void) {
    vga_menu_add_ex("---", NULL, NULL, NULL, VGA_COLOR_WHITE, MENU_ITEM_SEPARATOR);
}

// Add a checkbox item
void vga_menu_add_checkbox(const char* name, const char* tooltip, bool initial_state) {
    vga_menu_add_ex(name, tooltip, NULL, NULL, VGA_COLOR_WHITE, MENU_ITEM_CHECKBOX);
    if (root_menu_state.item_count > 0) {
        root_menu_state.items[root_menu_state.item_count - 1].checked = initial_state;
    }
}

// Toggle checkbox state
void vga_menu_toggle_checkbox(int index) {
    if (index >= 0 && index < root_menu_state.item_count) {
        MenuItemEx* item = &root_menu_state.items[index];
        if (item->flags & MENU_ITEM_CHECKBOX) {
            item->checked = !item->checked;
        }
    }
}

// Remove a menu item by index
void vga_menu_remove(int index) {
    if (index < 0 || index >= root_menu_state.item_count) return;
    
    for (int i = index; i < root_menu_state.item_count - 1; i++) {
        root_menu_state.items[i] = root_menu_state.items[i + 1];
    }
    root_menu_state.item_count--;
    
    if (menu_win_valid) {
        vga_destroy_window(&menu_win);
        menu_win_valid = 0;
    }
    
    if (root_menu_state.item_count > 0) {
        menu_win = create_menu_window_ex(&root_menu_state);
        if (menu_win.buffer) {
            menu_win_valid = 1;
            vga_menu_render_ex(&root_menu_state);
        }
    }
    
    if (root_menu_state.selected_index >= root_menu_state.item_count) {
        root_menu_state.selected_index = root_menu_state.item_count - 1;
    }
}

// Enhanced progress bar with more options
void vga_menu_progress_bar_ex(const char* title, const char* subtitle, 
                               int total_steps, int delay, char symbol,
                               enum vga_color bar_color) {
    if (!title) return;

    const int bar_length = 50;
    int title_len = strlen(title);
    int subtitle_len = subtitle ? strlen(subtitle) : 0;
    int max_text_len = (title_len > subtitle_len) ? title_len : subtitle_len;
    
    int win_width = max_text_len + bar_length + 10;
    if (win_width > VGA_WIDTH) win_width = VGA_WIDTH;
    if (win_width < 60) win_width = 60;
    
    int win_height = subtitle ? 5 : 4;
    int x = (VGA_WIDTH - win_width) / 2;
    int y = (VGA_HEIGHT - win_height) / 2;

    vga_window_t pb_win = vga_create_window_static(x, y, win_width, win_height, 
                                                    VGA_COLOR_WHITE, VGA_COLOR_BLACK, 
                                                    progress_bar_buffer, sizeof(progress_bar_buffer));

    if (!pb_win.buffer) return;

    for (int progress = 0; progress <= total_steps; progress++) {
        vga_win_clear(&pb_win);
        vga_win_draw_box(&pb_win, 0, 0, win_width, win_height);
        
        // Draw title
        uint8_t title_color = vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        int title_x = (win_width - title_len) / 2;
        for (int i = 0; i < title_len; i++) {
            pb_win.buffer[pb_win.width + title_x + i] = vga_entry(title[i], title_color);
        }
        
        // Draw subtitle if present
        int bar_row = 2;
        if (subtitle) {
            int subtitle_x = (win_width - subtitle_len) / 2;
            for (int i = 0; i < subtitle_len; i++) {
                pb_win.buffer[2 * pb_win.width + subtitle_x + i] = 
                    vga_entry(subtitle[i], vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            }
            bar_row = 3;
        }
        
        // Draw progress bar
        int bar_x = 2;
        uint8_t bar_color_code = vga_entry_color(bar_color, VGA_COLOR_BLACK);
        
        pb_win.buffer[bar_row * pb_win.width + bar_x++] = vga_entry('[', bar_color_code);
        
        int filled = (progress * bar_length) / total_steps;
        for (int k = 0; k < bar_length; k++) {
            char ch = (k < filled) ? symbol : '.';
            pb_win.buffer[bar_row * pb_win.width + bar_x++] = vga_entry(ch, bar_color_code);
        }
        
        pb_win.buffer[bar_row * pb_win.width + bar_x++] = vga_entry(']', bar_color_code);
        
        // Draw percentage
        char perc[10];
        int percent = (progress * 100) / total_steps;
        itoa(percent, perc, 10);
        int perc_len = strlen(perc);
        perc[perc_len++] = '%';
        perc[perc_len] = '\0';
        
        bar_x += 2;
        for (int i = 0; i < perc_len; i++) {
            pb_win.buffer[bar_row * pb_win.width + bar_x++] = 
                vga_entry(perc[i], vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        }
        
        vga_win_refresh(&pb_win);
        for (volatile int k = 0; k < delay * 10000; k++);
    }

    vga_destroy_window(&pb_win);
}

// Original progress bar for backward compatibility
void vga_menu_progress_bar(const char* title, int delay, char symbol) {
    vga_menu_progress_bar_ex(title, NULL, 50, delay, symbol, VGA_COLOR_GREEN);
}

// Show a message dialog
void vga_menu_show_dialog(const char* title, const char* message, enum vga_color color) {
    if (!title || !message) return;
    
    int title_len = strlen(title);
    int msg_len = strlen(message);
    int width = (title_len > msg_len ? title_len : msg_len) + 8;
    if (width > VGA_WIDTH) width = VGA_WIDTH;
    if (width < 30) width = 30;
    
    int height = 6;
    int x = (VGA_WIDTH - width) / 2;
    int y = (VGA_HEIGHT - height) / 2;
    
    vga_window_t dialog = vga_create_window_static(x, y, width, height,
                                                   VGA_COLOR_WHITE, VGA_COLOR_BLACK,
                                                   dialog_buffer, sizeof(dialog_buffer));
    if (!dialog.buffer) return;
    
    vga_win_clear(&dialog);
    vga_win_draw_box(&dialog, 0, 0, width, height);
    
    // Draw title
    uint8_t title_color = vga_entry_color(color, VGA_COLOR_BLACK);
    int title_x = (width - title_len) / 2;
    for (int i = 0; i < title_len && (title_x + i) < width - 1; i++) {
        dialog.buffer[dialog.width + title_x + i] = vga_entry(title[i], title_color);
    }
    
    // Draw message
    int msg_x = (width - msg_len) / 2;
    if (msg_x < 2) msg_x = 2;
    for (int i = 0; i < msg_len && (msg_x + i) < width - 2; i++) {
        dialog.buffer[3 * dialog.width + msg_x + i] = 
            vga_entry(message[i], vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
    
    // Draw OK prompt
    const char* ok_prompt = "Press any key...";
    int ok_len = strlen(ok_prompt);
    int ok_x = (width - ok_len) / 2;
    for (int i = 0; i < ok_len; i++) {
        dialog.buffer[4 * dialog.width + ok_x + i] = 
            vga_entry(ok_prompt[i], vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    }
    
    vga_win_refresh(&dialog);
    keyboard_read_input(); // Wait for key
    vga_destroy_window(&dialog);
}

// Example functions demonstrating new features
void vga_menu_example_progress(void) {
    vga_menu_progress_bar_ex("System Update", "Downloading packages...", 
                             100, 300, '=', VGA_COLOR_GREEN);
    vga_menu_show_dialog("Complete", "Update finished successfully!", VGA_COLOR_GREEN);
}

void vga_menu_example_themes(void) {
    const char* themes[] = {"Default", "Matrix", "Retro", "Fire"};
    for (int i = 0; i < 4; i++) {
        vga_menu_set_theme(i);
        vga_menu_render_ex(&root_menu_state);
        char msg[50];
        int j = 0;
        const char* txt = "Theme: ";
        while (*txt) msg[j++] = *txt++;
        txt = themes[i];
        while (*txt) msg[j++] = *txt++;
        msg[j] = '\0';
        vga_menu_show_dialog("Theme Preview", msg, VGA_COLOR_CYAN);
    }
    vga_menu_set_theme(0); // Reset to default
}

// Main menu loop - Enhanced version
void vga_menu_run(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    if (root_menu_state.item_count == 0 || !menu_win_valid || !menu_win.buffer) {
        return;
    }
    
    vga_menu_render_ex(&root_menu_state);
    int animation_counter = 0;
    
    while (1) {
        // Handle animation
        if (root_menu_state.animate_selection) {
            animation_counter++;
            if (animation_counter > ANIMATION_FRAME_DELAY) {
                animation_counter = 0;
                root_menu_state.animation_frame++;
                vga_menu_render_ex(&root_menu_state);
            }
        }
        
        int key = keyboard_key();
        if (key == -1) {
            continue;
        }
        
        int old_selection = root_menu_state.selected_index;
        
        switch (key) {
            // Navigation: Up (Arrow/W)
            case 0x48: // Up arrow
            case 0x11: // W key
                do {
                    root_menu_state.selected_index = 
                        (root_menu_state.selected_index > 0) ? 
                        root_menu_state.selected_index - 1 : 
                        root_menu_state.item_count - 1;
                } while ((root_menu_state.items[root_menu_state.selected_index].flags & MENU_ITEM_SEPARATOR) &&
                         root_menu_state.selected_index != old_selection);
                
                // Adjust scroll if needed
                if (root_menu_state.selected_index < root_menu_state.scroll_offset) {
                    root_menu_state.scroll_offset = root_menu_state.selected_index;
                }
                vga_menu_render_ex(&root_menu_state);
                break;
                
            // Navigation: Down (Arrow/S)
            case 0x50: // Down arrow
            case 0x1F: // S key
                do {
                    root_menu_state.selected_index = 
                        (root_menu_state.selected_index < root_menu_state.item_count - 1) ? 
                        root_menu_state.selected_index + 1 : 0;
                } while ((root_menu_state.items[root_menu_state.selected_index].flags & MENU_ITEM_SEPARATOR) &&
                         root_menu_state.selected_index != old_selection);
                
                // Adjust scroll if needed
                if (root_menu_state.selected_index >= 
                    root_menu_state.scroll_offset + root_menu_state.visible_items) {
                    root_menu_state.scroll_offset = 
                        root_menu_state.selected_index - root_menu_state.visible_items + 1;
                }
                vga_menu_render_ex(&root_menu_state);
                break;
                
            // Select/Execute (Space/Enter)
            case 0x39: // Space key
            case 0x1C: // Enter key
                if (root_menu_state.selected_index >= 0 && 
                    root_menu_state.selected_index < root_menu_state.item_count) {
                    MenuItemEx* item = &root_menu_state.items[root_menu_state.selected_index];
                    
                    if (!(item->flags & (MENU_ITEM_DISABLED | MENU_ITEM_SEPARATOR))) {
                        // Handle checkbox toggle
                        if (item->flags & MENU_ITEM_CHECKBOX) {
                            item->checked = !item->checked;
                            vga_menu_render_ex(&root_menu_state);
                        }
                        
                        // Execute action
                        if (item->action) {
                            item->action();
                            vga_menu_render_ex(&root_menu_state);
                        }
                        
                        // Execute callback
                        if (item->callback) {
                            item->callback(root_menu_state.selected_index, item->user_data);
                            vga_menu_render_ex(&root_menu_state);
                        }
                    }
                }
                break;
                
            // Toggle tooltip display (T key)
            case 0x14: // T key
                root_menu_state.show_tooltips = !root_menu_state.show_tooltips;
                vga_menu_render_ex(&root_menu_state);
                break;
                
            // Toggle animation (A key)
            case 0x1E: // A key
                root_menu_state.animate_selection = !root_menu_state.animate_selection;
                vga_menu_render_ex(&root_menu_state);
                break;
                
            // Exit (ESC)
            case 0x76: // ESC key
            case 0x01: // ESC scancode
                vga_menu_destroy(NULL);
                return;
        }
    }
}

// Cleanup function
void vga_menu_destroy(multi_panel_menu_t* menu) {
    (void)menu;
    if (menu_win_valid) {
        vga_destroy_window(&menu_win);
        menu_win_valid = 0;
    }
    root_menu_state.item_count = 0;
    root_menu_state.selected_index = 0;
}

// Backward compatibility wrapper
void vga_menu_render(multi_panel_menu_t* menu) {
    (void)menu;
    vga_menu_render_ex(&root_menu_state);
}