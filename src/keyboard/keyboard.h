#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h> // For bool type
#include <stddef.h>  // For size_t
#include <stdint.h> 
typedef struct {
    const char* name;
    const char* description;
    void (*execute)(int, char*[]); // Update to accept arguments
} Command;

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
