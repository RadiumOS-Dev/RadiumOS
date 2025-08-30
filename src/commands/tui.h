#ifndef TUI_H
#define TUI_H

#include <stdint.h>

#define COMMAND_BUFFER_SIZE 256 // Buffer size for string conversion
#define MAX_MENU_ITEMS 10        // Maximum number of menu items

typedef void (*MenuFunction)(); // Type for menu item functions

// Structure to hold menu item information
typedef struct {
    const char* name;      // Name of the menu item
    MenuFunction action;       // Function to call when selected
    int color;             // Color for the menu item
} MenuItem;

// Function to add a menu option
void add(const char* name, MenuFunction action, int color);

// Function to remove a menu option by index
void rem(int index);

// Function to render the menu
void r();

// Function to display a progress bar
void pb(const char* text, int delay, char symbol);

// Example function to be called from the menu
void ef();

// Function to run the TUI loop
void tui(int argc, char* argv[]);

#endif // TUI_H
