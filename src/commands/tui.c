#include "../terminal/terminal.h"
#include "../keyboard/keyboard.h"
#include "../utility/utility.h"
#include "../cpu/cpu.h"
#include "../timers/timer.h"
#include "tui.h"

MenuItem m[MAX_MENU_ITEMS]; // Menu items
int s = 0, c = 0;           // Selection index and count

void add(const char* n, MenuFunction a, int color) {
    if (c < MAX_MENU_ITEMS) {
        m[c++] = (MenuItem){n, a, color};
    }
}

void rem(int i) {
    if (i >= 0 && i < c) {
        for (; i < c - 1; i++) m[i] = m[i + 1];
        c--;
    }
}

void r() {
    terminal_clear();
    char b[COMMAND_BUFFER_SIZE];
    for (int i = 0; i < c; i++) {
        int j = 0;
        // Set the color for the menu item
        terminal_setcolor(m[i].color);
        
        if (i == s) {
            b[j++] = '>'; // Indicate selection
        }
        
        for (const char* item = m[i].name; *item; item++) b[j++] = *item;
        
        if (i == s) {
            b[j++] = '<'; // Indicate selection
        }
        
        b[j++] = '\n'; 
        b[j] = '\0'; 
        print(b);
    }
}

void pb(const char* t, int d, char s) {
    const int l = 50; 
    char b[COMMAND_BUFFER_SIZE];
    for (int i = 0; i <= l; i++) {
        terminal_clear();
        int j = 0;
        for (const char* txt = t; *txt; txt++) b[j++] = *txt;
        b[j++] = '\n'; b[j++] = '['; 
        for (int k = 0; k < l; k++) b[j++] = (k < i) ? s : ' ';
        b[j++] = ']'; b[j] = '\0'; 
        print(b);
        char p[10]; itoa((i * 100) / l, p, 10); p[2] = '%'; p[3] = '\0'; 
        print(p);
        for (volatile int k = 0; k < d * 100000; k++);
    }
}

void ef() {
    pb("Hello, World", 1000, '#');
}

void tui(int argc, char* argv[]) {
    while (1) {
        r(); 
        int res = keyboard_key(); 
        switch (res) {
            case 0x11: // W key
                s = (s > 0) ? s - 1 : c - 1; // Scroll up, wrap to bottom
                break;
            case 0x1F: // S key
                s = (s < c - 1) ? s + 1 : 0; // Scroll down, wrap to top
                break;
            case 0x39: // Space key
                if (m[s].action) m[s].action(); // Call the action
                break;
        }
    }
}
