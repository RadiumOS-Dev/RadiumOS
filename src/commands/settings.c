#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../keyboard/keyboard.h"
#include "settings.h"
// Define the variables here
bool is_verbose;
bool is_debug;
void settings_command(int argc, char* argv[]) {
    char input[COMMAND_BUFFER_SIZE];
    while (1) {
        // Display current settings
        print("\n=== SETTINGS ===\n");
        print("Debug: ");
        if (is_debug) {
            print("ON\n");
        } else {
            print("OFF\n");
        }
        
        print("Verbose: ");
        if (is_verbose) {
            print("ON\n");
        } else {
            print("OFF\n");
        }
        print("================\n");
        print("Enter command: ");
        
        keyboard_input(input);
        
        if (strcmp(input, "help") == 0) {
            print("\n=== HELP ===\n");
            print("`help`    - Shows this message\n");
            print("`debug`   - Toggles debug mode on/off\n");
            print("`verbose` - Toggles verbose mode on/off\n");
            print("`status`  - Shows current settings\n");
            print("`exit`    - Exit settings menu\n");
            print("`clear`   - Clear screen\n");
            print("=============\n");
        } 
        else if (strcmp(input, "debug") == 0) {
            is_debug = !is_debug;
            print("Debug mode ");
            if (is_debug) {
                print("ENABLED\n");
            } else {
                print("DISABLED\n");
            }
        }
        else if (strcmp(input, "verbose") == 0) {
            is_verbose = !is_verbose;
            print("Verbose mode ");
            if (is_verbose) {
                print("ENABLED\n");
            } else {
                print("DISABLED\n");
            }
        }
        else if (strcmp(input, "status") == 0) {
            // Status is shown at the top of the loop, so just continue
            continue;
        }
        else if (strcmp(input, "clear") == 0) {
            terminal_clear_inFunction();
        }
        else if (strcmp(input, "exit") == 0) {
            print("Exiting settings...\n");
            break;
        }
        else if (strlen(input) > 0) {
            print("Unknown command: ");
            print(input);
            print("\nType 'help' for available commands.\n");
        }
        terminal_clear_inFunction();
    }
}