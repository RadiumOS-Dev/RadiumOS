#include "../terminal/terminal.h"
#include "echo.h"

void echo_command(int argc, char* argv[]) {
    if (argc < 2) {
        print("Usage: echo <text>\n");
        return;
    }
    print("\n");
    // Print all arguments after the command
    for (int i = 1; i < argc; i++) {
        print(argv[i]);
        if (i < argc - 1) {
            print(" "); // Add a space between arguments
        }
    }
    print("\n");
}
