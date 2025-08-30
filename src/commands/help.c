#include "../terminal/terminal.h"
#include "../keyboard/keyboard.h" // Include the keyboard header
#include "help.h"
void help_command(int argc, char* argv[]) {
    print("\n\tAvailable commands:\n");
    for (size_t i = 0; i < command_count; i++) {
        print("\t\t\t");
        print(commands[i].name); // Print command name
        print(": "); 
        print(commands[i].description); // Print command description
        print("\n");
    }
}
