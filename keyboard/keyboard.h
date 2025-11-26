#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h> // For bool type
#include <stddef.h>  // For size_t
#include <stdint.h> 

typedef struct {
    const char *name;
    const char* description;
    void (*execute)(int argc, char *argv[]);
    bool requires_sudo;  // Command must run as sudo
    bool allow_sudo;     // Command can run as sudo
} Command;

#define MAX_HISTORY 10 // Maximum number of commands to store in history
#define COMMAND_BUFFER_SIZE 256
#define MAX_COMMANDS 100 // Maximum number of commands
#define MAX_ARGUMENTS 10 // Maximum number of arguments per command

#define PRIVILEGE_PORT 0x60  // Example port for privilege management
#define SUDO_LEVEL 0x01      // Privilege level for sudo/root
#define USER_LEVEL 0x00      // Normal user privilege level
bool is_key_down(uint8_t scancode);
// Function prototypes
bool is_key_pressed();
void execute_command(const char* command);
void keyboard_handler();
void keyboard_task();
void keyboard_await();
int register_command(const char* name, const char* description, void (*execute)(int, char*[]));
int keyboard_input(char* userinput);
void keyboard_input_secure(char* userinput);
void keyboard_read_input();
uint8_t keyboard_wait_for_key(bool dump_scancode);
uint8_t keyboard_key();
void toggle_caps_lock();
void set_keyboard_leds(uint8_t leds);
char keyboard_to_char(uint8_t scancode, bool shift, bool caps_lock);
void execute_c(int argc, char *argv[]);
void execute_command_as_sudo_user(const char *command);
// Constants
#define MAX_HISTORY 10 // Maximum number of commands to store in history
#define COMMAND_BUFFER_SIZE 1000 // Size of the command buffer
#define MAX_COMMANDS 100 // Maximum number of commands
#define LED_CAPS_LOCK = 4


// External variables (if needed)

// External variables for commands
extern Command commands[MAX_COMMANDS]; // Array to hold commands
extern size_t command_count; // Number of registered commands

#endif // KEYBOARD_H
