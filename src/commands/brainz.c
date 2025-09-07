#include "brainz.h"
#include "../cpu/cpu.h"
#include "../terminal/terminal.h" 
#include "../vga/vga.h"
#include "../keyboard/keyboard.h"
#include "../timers/timer.h"
#include "../utility/utility.h"
#include "reboot.h"

void brains_command(int argc, char* argv[]) {
    char input[COMMAND_BUFFER_SIZE];
    terminal_clear_inFunction();
    print("Color Changer v1\n`help` for color codes!\n`presets` Load a preset color combo!\n`quit` To quit.");

    while (1) {
        keyboard_input(input); 
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "help") == 0) {
            print("\nVGA_COLOR_BLACK = 0");
            print("\nVGA_COLOR_BLUE = 1");
            print("\nVGA_COLOR_GREEN = 2");
            print("\nVGA_COLOR_CYAN = 3");
            print("\nVGA_COLOR_RED = 4");
            print("\nVGA_COLOR_MAGENTA = 5");
            print("\nVGA_COLOR_BROWN = 6");
            print("\nVGA_COLOR_LIGHT_GREY = 7");
            print("\nVGA_COLOR_DARK_GREY = 8");
            print("\nVGA_COLOR_LIGHT_BLUE = 9");
            print("\nVGA_COLOR_LIGHT_GREEN = 10");
            print("\nVGA_COLOR_LIGHT_CYAN = 11");
            print("\nVGA_COLOR_LIGHT_RED = 12");
            print("\nVGA_COLOR_LIGHT_MAGENTA = 13");
            print("\nVGA_COLOR_LIGHT_BROWN = 14");
            print("\nVGA_COLOR_WHITE = 15\n");
        } else if (strcmp(input, "presets") == 0) {
            print("Choose a preset color combo:\n");
            print("1.) Blue Background, Black Text\n");
            print("2.) Red Background, White Text\n");
            print("3.) Green Background, Black Text\n");
            print("4.) Black Background, Light Grey Text\n");
            print("5.) Cyan Background, Dark Grey Text\n");
            print("6.) Magenta Background, ?? Text\n");
            print("7.) Yellow Background, Black Text\n");
            print("8.) Light Blue Background, Dark Grey Text\n");
            print("9.) Light Green Background, Black Text\n");
            print("10.) White Background, Blue Text\n");

            keyboard_input(input);
            // Remove newline character from input if present
            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "1") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_BLUE));
                terminal_clear_inFunction();
                print("Changed to Blue Background, Black Text.\n");
            } else if (strcmp(input, "2") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
                terminal_clear_inFunction();
                print("Changed to Red Background, White Text.\n");
            } else if (strcmp(input, "3") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_GREEN));
                terminal_clear_inFunction();
                print("Changed to Green Background, Black Text.\n");
            } else if (strcmp(input, "4") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
                terminal_clear_inFunction();
                print("Changed to Black Background, Light Grey Text.\n");
            } else if (strcmp(input, "5") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_CYAN));
                terminal_clear_inFunction();
                print("Changed to Cyan Background, Dark Grey Text.\n");
            } else if (strcmp(input, "6") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_MAGENTA));
                terminal_clear_inFunction();
                print("Changed to Magenta Background, ?? Text.\n");
            } else if (strcmp(input, "7") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_CYAN));
                terminal_clear_inFunction();
                print("Changed to Black Background, Cyan Text.\n");
            } else if (strcmp(input, "8") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_LIGHT_BLUE));
                terminal_clear_inFunction();
                print("Changed to Light Blue Background, Dark Grey Text.\n");
            } else if (strcmp(input, "9") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN));
                terminal_clear_inFunction();
                print("Changed to Light Green Background, Black Text.\n");
            } else if (strcmp(input, "10") == 0) {
                terminal_setcolor(vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_WHITE));
                terminal_clear_inFunction();
                print("Changed to White Background, Blue Text.\n");
            } else {
                print("Invalid preset selection.\n");
            }
        } else if (strcmp(input, "quit") == 0) {
            print("Bye :)\n");
            break;
        } else {
            int result = string_to_int(input);
            terminal_setcolor(result);
        }
    }  
}
