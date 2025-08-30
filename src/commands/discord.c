// Just a simple discord like app

#include "../keyboard/keyboard.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"

#include "tui.h"

void dumpServers() {
    //
}

void discord_command(int argc, char* argv[]) {
    char username[COMMAND_BUFFER_SIZE];char password[COMMAND_BUFFER_SIZE];int i = 0;
    print("\nUsername: ");while (1) if (i > 5)print("Max amount of tries !\nThis action is reported.");keyboard_input(username);if (strcmp(username, "root") == 0)print("\nPassword: ");keyboard_input_secure(password);if (strcmp(password, "toor") == 0)return;else print("\n\nInvalid Username/password!\n\n"); i = i + 1; 
}