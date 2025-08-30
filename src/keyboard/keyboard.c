#include "keyboard.h"
#include <stddef.h>  // For size_t
#include "../terminal/terminal.h" // For terminal functions
#include "../utility/utility.h" // For utility functions
#include "../errors/error.h" // For error handling functions
#include "../io/io.h" // For I/O operations
#include "../scheduler/task.h"
#include "../login/login.h"
#include <stdint.h> 
#define MAX_HISTORY 10 // Maximum number of commands to store in history
#define COMMAND_BUFFER_SIZE 256
#define MAX_COMMANDS 100 // Maximum number of commands
#define MAX_ARGUMENTS 10 // Maximum number of arguments per command

char command_history[MAX_HISTORY][COMMAND_BUFFER_SIZE];
size_t history_count = 0;
int current_history_index = -1; // Current position in history (-1 means not browsing)

Command commands[MAX_COMMANDS]; // Array to hold commands
size_t command_count = 0; // Number of registered commands

// Shift state variable
bool shift_active = false;
void display_prompt() {
    print("<");
    print("thorne");
    print("> @Radium# ");
}

int register_command(const char* name, const char* description, void (*execute)(int, char*[])) {
    if (command_count >= MAX_COMMANDS) {
        return 0; // Failed: maximum commands reached
    }
    
    if (!name || !description || !execute) {
        return 0; // Failed: invalid parameters
    }
    
    // Check if command already exists
    for (int i = 0; i < command_count; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            return 0; // Failed: command already exists
        }
    }
    
    commands[command_count].name = name;
    commands[command_count].description = description;
    commands[command_count].execute = execute;
    command_count++;
    
    return 1; // Success
}

// Function to add command to history
void add_to_history(const char* command) {
    if (strlen(command) == 0) return; // Don't add empty commands
    
    // Shift existing commands down if we're at max capacity
    if (history_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strncpy(command_history[i], command_history[i + 1], COMMAND_BUFFER_SIZE);
        }
        strncpy(command_history[MAX_HISTORY - 1], command, COMMAND_BUFFER_SIZE);
    } else {
        strncpy(command_history[history_count], command, COMMAND_BUFFER_SIZE);
        history_count++;
    }
    current_history_index = -1; // Reset history browsing
}
// Function to clear current line and redraw command
void redraw_command_line(const char* command, size_t* command_length) {
    // Move cursor to beginning of line
    terminal_putchar('\r');

    for (int i = 0; i < 79; i++) {
        terminal_putchar(' ');
    }
    
    // Move cursor back to beginning of line
    terminal_putchar('\r');
    
    // Redraw prompt
    display_prompt();
    
    // Print the new command
    for (size_t i = 0; i < strlen(command); i++) {
        terminal_putchar(command[i]);
    }
    
    *command_length = strlen(command);
}



// Function to check if a key is pressed
bool is_key_pressed() {
    return (port_byte_in(0x64) & 0x01) != 0; // Check if a key is pressed
}

// Function to wait for a key press
void keyboard_await() {
    print("Press any key to continue...\n"); // Prompt the user
    while (!is_key_pressed()) {
        // Wait until a key is pressed
    }
    // Clear the key from the buffer
    port_byte_in(0x60); // Read from the keyboard port to clear the interrupt
}

// Function to execute commands
void execute_command(const char* command) {
    if (strcmp(command, "") == 0) return; // Prevent empty command execution

    // Add command to history
    add_to_history(command);

    // Split command into arguments
    char* argv[MAX_ARGUMENTS];
    int argc = 0;
    char* token = strtok((char*)command, " "); // Tokenize the command by spaces

    while (token != NULL && argc < MAX_ARGUMENTS) {
        argv[argc++] = token; // Store each argument
        token = strtok(NULL, " "); // Get the next token
    }

    // Check if the command matches any loaded command
    for (size_t i = 0; i < command_count; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].execute(argc, argv); // Call the command's execute function with arguments
            return;
        }
    }

    // If no command matched, handle unknown command
    print("\n! unknown command !\n");
}

// Keyboard map for character translation
const char keyboard_map[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c',
    'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' ', 0, 0, 0, 0, 0, 0,
};

// Shifted keyboard map for uppercase characters
const char shifted_keyboard_map[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C',
    'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' ', 0, 0, 0, 0, 0, 0,
};


// Function to handle arrow key sequences
bool handle_arrow_keys(uint8_t scan_code, char* command_buffer, size_t* command_length) {
    static uint8_t arrow_sequence = 0; // Track arrow key sequence state
    
    if (scan_code == 0xE0) { // Extended key prefix
        arrow_sequence = 1;
        return true; // Consumed the key
    }
    
    if (arrow_sequence == 1) {
        arrow_sequence = 0; // Reset sequence state
        
        if (scan_code == 0x48) { // Up arrow
            if (history_count > 0) {
                if (current_history_index == -1) {
                    current_history_index = history_count - 1;
                } else if (current_history_index > 0) {
                    current_history_index--;
                }
                
                // Copy history command to buffer and redraw
                strncpy(command_buffer, command_history[current_history_index], COMMAND_BUFFER_SIZE);
                command_buffer[COMMAND_BUFFER_SIZE - 1] = '\0';
                redraw_command_line(command_buffer, command_length);
            }
            return true; // Consumed the key
        }
        else if (scan_code == 0x50) { // Down arrow
            if (history_count > 0 && current_history_index != -1) {
                if (current_history_index < history_count - 1) {
                    current_history_index++;
                    strncpy(command_buffer, command_history[current_history_index], COMMAND_BUFFER_SIZE);
                    command_buffer[COMMAND_BUFFER_SIZE - 1] = '\0';
                } else {
                    // Go past last history item (clear command)
                    current_history_index = -1;
                    command_buffer[0] = '\0';
                }
                redraw_command_line(command_buffer, command_length);
            }
            return true; // Consumed the key
        }
    }
    
    return false; // Key not consumed
}

// Add this global variable at the top with your other globals
bool caps_lock_active = false;

// Keyboard handler function
void keyboard_handler() {
    static char command_buffer[COMMAND_BUFFER_SIZE];
    static size_t command_length = 0;

    if (is_key_pressed()) {
        uint8_t scan_code = port_byte_in(0x60); // Read from keyboard port
        
        // Handle arrow keys first
        if (handle_arrow_keys(scan_code, command_buffer, &command_length)) {
            return; // Arrow key was handled
        }
        
        if (scan_code & 0x80) { // Check if it's a break code (key release)
            if (scan_code == 0xAA || scan_code == 0xB6) { // Left Shift or Right Shift release
                shift_active = false; // Deactivate shift state
            }
        } else { // Key press (make code)
            if (scan_code == 0x2A || scan_code == 0x36) { // Left Shift or Right Shift press
                shift_active = true; // Activate shift state
            } else if (scan_code == 0x3A) { // Caps Lock key press
                if (shift_active) { // Only toggle if shift is held
                    caps_lock_active = !caps_lock_active; // Toggle caps lock state
                    // Optional: provide visual feedback
                    print(caps_lock_active ? " [CAPS ON] " : " [CAPS OFF] ");
                }
            } else if (scan_code < sizeof(keyboard_map)) {
                char c = 0;
                
                if (scan_code == 0x1C) { // Enter key
                    command_buffer[command_length] = '\0'; // Null-terminate the command
                    execute_command(command_buffer); // Execute the command
                    command_length = 0; // Reset command length
                    command_buffer[0] = '\0'; // Clear buffer
                    current_history_index = -1; // Reset history browsing
                    terminal_putchar('\n'); // Move to the next line
                    display_prompt(); // Display the prompt again
                } else if (scan_code == 0x0E) { // Backspace key
                    if (command_length > 0) {
                        command_length--; // Decrease command length
                        command_buffer[command_length] = '\0'; // Update buffer
                        current_history_index = -1; // Reset history browsing when editing
                        terminal_putchar('\b'); // Move cursor back
                        terminal_putchar(' '); // Clear the character
                        terminal_putchar('\b'); // Move cursor back again
                    }
                } else {
                    // Get the base character
                    char base_char = keyboard_map[scan_code];
                    char shifted_char = shifted_keyboard_map[scan_code];
                    
                    if (base_char != 0) {
                        // For letters (a-z), apply caps lock logic
                        if (base_char >= 'a' && base_char <= 'z') {
                            // XOR logic: caps lock XOR shift = uppercase
                            if (caps_lock_active ^ shift_active) {
                                c = base_char - 'a' + 'A'; // Convert to uppercase
                            } else {
                                c = base_char; // Keep lowercase
                            }
                        } 
                        // For non-letters, only shift matters
                        else {
                            if (shift_active) {
                                c = shifted_char;
                            } else {
                                c = base_char;
                            }
                        }
                        
                        // Add character to buffer if valid
                        if (c != 0 && command_length < COMMAND_BUFFER_SIZE - 1) {
                            command_buffer[command_length++] = c;
                            command_buffer[command_length] = '\0';
                            current_history_index = -1; // Reset history browsing when typing
                            terminal_putchar(c);
                        }
                    }
                }
            }
        }
    }
}


// Function to read keyboard input
void keyboard_read_input(uint32_t id) {
    display_prompt(); // Display prompt for the first command
    while (true) {
        keyboard_handler(); // Call the keyboard handler function when a key is pressed
    }
}

void keyboard_input_secure(char* userinput) {
    static char command_buffer[COMMAND_BUFFER_SIZE] = {0}; // Initialize buffer to zero
    size_t command_length = 0;
    while (true) {
        if (is_key_pressed()) {
            uint8_t scan_code = port_byte_in(0x60); // Read from keyboard port
            if (scan_code < sizeof(keyboard_map)) {
                if (scan_code & 0x80) { // Check if it's a break code
                    // Handle key release
                    if (scan_code == 0xAA || scan_code == 0xB6) { // Left Shift or Right Shift release
                        shift_active = false; // Deactivate shift state
                    }
                } else {
                    // Check for shift key press
                    if (scan_code == 0x2A || scan_code == 0x36) { // Left Shift or Right Shift press
                        shift_active = true; // Activate shift state
                    } else {
                        char c;
                        if (shift_active) {
                            c = shifted_keyboard_map[scan_code]; // Get character from shifted keyboard map
                        } else {
                            c = keyboard_map[scan_code]; // Get character from keyboard map
                        }
                        if (scan_code == 0x1C) { // Enter key
                            command_buffer[command_length] = '\0'; // Null-terminate the command
                            strncpy(userinput, command_buffer, COMMAND_BUFFER_SIZE); // Copy to userinput
                            userinput[COMMAND_BUFFER_SIZE - 1] = '\0'; // Ensure null-termination
                            command_length = 0; // Reset command length
                            terminal_putchar('\n'); // Move to the next line
                            secure_clear(command_buffer, sizeof(command_buffer)); // Clear command buffer
                            return; // Exit the function after getting input
                        } else if (scan_code == 0x0E) { // Backspace key
                            if (command_length > 0) {
                                command_length--; // Decrease command length
                                terminal_putchar('\b'); // Move cursor back
                                terminal_putchar(' '); // Clear the character
                                terminal_putchar('\b'); // Move cursor back again
                            }
                        } else if (c != 0 && command_length < COMMAND_BUFFER_SIZE - 1) { // Check buffer size and valid character
                            command_buffer[command_length++] = c; // Add character to command buffer
                            terminal_putchar('*'); // Print the masked character
                        }
                    }
                }
            }
        }
    }
}

void keyboard_input(char* userinput) {
    static char command_buffer[COMMAND_BUFFER_SIZE];
    size_t command_length = 0;

    while (true) {
        if (is_key_pressed()) {
            uint8_t scan_code = port_byte_in(0x60); // Read from keyboard port
            if (scan_code < sizeof(keyboard_map)) {
                if (scan_code & 0x80) { // Check if it's a break code
                    // Handle key release
                    if (scan_code == 0xAA || scan_code == 0xB6) { // Left Shift or Right Shift release
                        shift_active = false; // Deactivate shift state
                    }
                } else {
                    // Check for shift key press
                    if (scan_code == 0x2A || scan_code == 0x36) { // Left Shift or Right Shift press
                        shift_active = true; // Activate shift state
                    } else {
                        char c;
                                                if (shift_active) {
                            c = shifted_keyboard_map[scan_code]; // Get character from shifted keyboard map
                        } else {
                            c = keyboard_map[scan_code]; // Get character from keyboard map
                        }

                        if (scan_code == 0x1C) { // Enter key
                            command_buffer[command_length] = '\0'; // Null-terminate the command
                            strncpy(userinput, command_buffer, COMMAND_BUFFER_SIZE); // Copy to userinput
                            userinput[COMMAND_BUFFER_SIZE - 1] = '\0'; // Ensure null-termination
                            command_length = 0; // Reset command length
                            terminal_putchar('\n'); // Move to the next line
                            return; // Exit the function after getting input
                        } else if (scan_code == 0x0E) { // Backspace key
                            if (command_length > 0) {
                                command_length--; // Decrease command length
                                terminal_putchar('\b'); // Move cursor back
                                terminal_putchar(' '); // Clear the character
                                terminal_putchar('\b'); // Move cursor back again
                            }
                        } else if (c != 0 && command_length < COMMAND_BUFFER_SIZE - 1) { // Check buffer size and valid character
                            command_buffer[command_length++] = c; // Add character to command buffer
                            terminal_putchar(c); // Print the character
                        }
                    }
                }
            }
        }
    }
}

uint8_t keyboard_wait_for_key(bool dump_scancode) {
    while (true) {
        if (is_key_pressed()) {
            uint8_t scan_code = port_byte_in(0x60); // Read from keyboard port
            if (dump_scancode) {
                // Print the scan code if the flag is set
                print("Scan code: ");
                print_hex(scan_code); // Assuming print_hex is a function to print hex values
                print("\n");
            }
            if (scan_code < sizeof(keyboard_map)) {
                if (!(scan_code & 0x80)) { // Check if it's not a break code
                    char c;
                    if (scan_code == 0x2A || scan_code == 0x36) { // Shift key
                        shift_active = true; // Set shift active
                    } else {
                        // Get character from the keyboard map
                        c = shift_active ? shifted_keyboard_map[scan_code] : keyboard_map[scan_code];
                        return c; // Return the character
                    }
                }
            }
        }
    }
}

uint8_t scancode_keyboard_interrupt() {
    static bool ctrl_pressed = false; // Track if Ctrl is currently pressed
    static bool c_pressed = false; // Track if C is currently pressed
    if (is_key_pressed()) {
        uint8_t scan_code = port_byte_in(0x60); // Read from keyboard port
        if (scan_code == 0x1D) { // Ctrl key pressed
            ctrl_pressed = true; // Set Ctrl as pressed
        } else if (scan_code == 0x1D | 0x80) { // Ctrl key released
            ctrl_pressed = false; // Reset Ctrl as released
        } else if (scan_code == 0x2E) { // C key pressed
            if (ctrl_pressed) {
                c_pressed = true; // Set C as pressed while Ctrl is held
            }
        } else if (scan_code == 0x2E | 0x80) { // C key released
            if (c_pressed && ctrl_pressed) {
                c_pressed = false; // Reset C pressed state
                return 1; // Ctrl+C detected
            }
        }
    }
    return 0; // Ctrl+C not detected
}

uint8_t keyboard_key() {
    keyboard_await();
    print("\n");
    return port_byte_in(0x60); 
}
