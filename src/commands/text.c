#include "../keyboard/keyboard.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../mpop/mpop.h"

#define MAX_PROGRAM_SIZE 4096
#define MAX_LINES 100

// Static buffer for the program being edited
static char program_buffer[MAX_PROGRAM_SIZE];
static char* program_lines[MAX_LINES];
static int line_count = 0;
static int current_line = 0;
static bool program_modified = false;

// Initialize the text editor
void textspace_init() {
    // Clear program buffer
    for (int i = 0; i < MAX_PROGRAM_SIZE; i++) {
        program_buffer[i] = '\0';
    }
    
    // Clear line pointers
    for (int i = 0; i < MAX_LINES; i++) {
        program_lines[i] = NULL;
    }
    
    line_count = 0;
    current_line = 0;
    program_modified = false;
}

// Add a line to the program
void textspace_add_line(const char* line) {
    if (line_count >= MAX_LINES - 1) {
        print("Maximum lines reached!\n");
        return;
    }
    
    // Find space in buffer for new line
    int buffer_pos = 0;
    for (int i = 0; i < line_count; i++) {
        if (program_lines[i]) {
            buffer_pos += strlen(program_lines[i]) + 1; // +1 for null terminator
        }
    }
    
    // Check if we have enough space
    int line_len = strlen(line);
    if (buffer_pos + line_len + 1 >= MAX_PROGRAM_SIZE) {
        print("Program buffer full!\n");
        return;
    }
    
    // Copy line to buffer
    program_lines[line_count] = &program_buffer[buffer_pos];
    strcpy(program_lines[line_count], line);
    line_count++;
    program_modified = true;
}

// Display the current program
void textspace_display_program() {
    terminal_clear_inFunction();
    print("=== MPOP Text Editor ===\n");
    print("Lines: ");
    print_integer(line_count);
    if (program_modified) {
        print(" (modified)");
    }
    print("\n\n");
    
    for (int i = 0; i < line_count; i++) {
        if (i == current_line) {
            terminal_setcolor(VGA_COLOR_BROWN);
            print(">");
        } else {
            print(" ");
        }
        
        print_integer(i + 1);
        print(": ");
        
        if (program_lines[i]) {
            print(program_lines[i]);
        }
        print("\n");
        
        if (i == current_line) {
            terminal_setcolor(VGA_COLOR_WHITE);
        }
    }
    
    print("Commands:\n");
    print("  /run           - Execute the program\n");
    print("  /load <name>   - Load example (hello/fib/count/calc)\n");
    print("  /clear         - Clear all lines\n");
    print("  /del           - Delete current line\n");
    print("  /up            - Move cursor up\n");
    print("  /down          - Move cursor down\n");
    print("  /list          - Show program listing\n");
    print("  /help          - Show this help\n");
    print("  /exit          - Exit editor\n");
}

// Convert program lines to single string for MPOP
char* textspace_get_program_string() {
    static char full_program[MAX_PROGRAM_SIZE];
    int pos = 0;
    
    full_program[0] = '\0';
    
    for (int i = 0; i < line_count; i++) {
        if (program_lines[i] && pos < MAX_PROGRAM_SIZE - 2) {
            int line_len = strlen(program_lines[i]);
            if (pos + line_len + 1 < MAX_PROGRAM_SIZE) {
                strcat(full_program, program_lines[i]);
                strcat(full_program, "\n");
                pos += line_len + 1;
            }
        }
    }
    
    return full_program;
}

// Load example programs
void textspace_load_example(const char* example) {
    textspace_init(); // Clear current program
    
    const char* program_text = NULL;
    
    if (strcmp(example, "hello") == 0) {
        program_text = 
            "MOV R0, 72\n"      // 'H'
            "PRINTC R0\n"
            "MOV R0, 101\n"     // 'e'
            "PRINTC R0\n"
            "MOV R0, 108\n"     // 'l'
            "PRINTC R0\n"
            "PRINTC R0\n"       // 'l'
            "MOV R0, 111\n"     // 'o'
            "PRINTC R0\n"
            "MOV R0, 32\n"      // ' '
            "PRINTC R0\n"
            "MOV R0, 87\n"      // 'W'
            "PRINTC R0\n"
            "MOV R0, 111\n"     // 'o'
            "PRINTC R0\n"
            "MOV R0, 114\n"     // 'r'
            "PRINTC R0\n"
            "MOV R0, 108\n"     // 'l'
            "PRINTC R0\n"
            "MOV R0, 100\n"     // 'd'
            "PRINTC R0\n"
            "MOV R0, 10\n"      // '\n'
            "PRINTC R0\n"
            "HALT\n";
    } else if (strcmp(example, "fib") == 0) {
        program_text = 
            "MOV R0, 0\n"       // First fibonacci number
            "MOV R1, 1\n"       // Second fibonacci number
            "MOV R2, 10\n"      // Counter
            "loop:\n"
            "PRINT R0\n"        // Print current number
            "PRINTC 32\n"       // Print space
            "ADD R3, R0, R1\n"  // R3 = R0 + R1
            "MOV R0, R1\n"      // R0 = R1
            "MOV R1, R3\n"      // R1 = R3
            "DEC R2\n"          // Decrement counter
            "JNZ loop\n"        // Jump if not zero
            "PRINTC 10\n"       // Print newline
            "HALT\n";
    } else if (strcmp(example, "count") == 0) {
        program_text = 
            "MOV R0, 1\n"       // Counter
            "MOV R1, 10\n"      // Limit
            "loop:\n"
            "PRINT R0\n"        // Print counter
            "PRINTC 32\n"       // Print space
            "INC R0\n"          // Increment counter
            "CMP R0, R1\n"      // Compare with limit
            "JLE loop\n"        // Jump if less or equal
            "PRINTC 10\n"       // Print newline
            "HALT\n";
    } else if (strcmp(example, "calc") == 0) {
        program_text = 
            "MOV R0, 15\n"      // First number
            "MOV R1, 7\n"       // Second number
            "ADD R2, R0, R1\n"  // Addition
            "SUB R3, R0, R1\n"  // Subtraction
            "MUL R4, R0, R1\n"  // Multiplication
            "DIV R5, R0, R1\n"  // Division
            "PRINT R2\n"        // Print sum
            "PRINTC 32\n"
            "PRINT R3\n"        // Print difference
            "PRINTC 32\n"
            "PRINT R4\n"        // Print product
            "PRINTC 32\n"
            "PRINT R5\n"        // Print quotient
            "PRINTC 10\n"
            "HALT\n";
    } else {
        print("Unknown example. Available: hello, fib, count, calc\n");
        return;
    }
    
    // Parse the program text into lines
    char temp_line[256];
    int line_pos = 0;
    int text_pos = 0;
    
    while (program_text[text_pos]) {
        if (program_text[text_pos] == '\n') {
            temp_line[line_pos] = '\0';
            if (line_pos > 0) {
                textspace_add_line(temp_line);
            }
            line_pos = 0;
        } else {
            if (line_pos < 255) {
                temp_line[line_pos++] = program_text[text_pos];
            }
        }
        text_pos++;
    }
    
    // Add final line if it doesn't end with newline
    if (line_pos > 0) {
        temp_line[line_pos] = '\0';
        textspace_add_line(temp_line);
    }
    
    print("Loaded example: ");
    print(example);
    print("\n");
}

// Delete current line
void textspace_delete_line() {
    if (line_count == 0) {
        print("No lines to delete\n");
        return;
    }
    
    // Shift lines up
    for (int i = current_line; i < line_count - 1; i++) {
        program_lines[i] = program_lines[i + 1];
    }
    
    line_count--;
    program_modified = true;
    
    if (current_line >= line_count && line_count > 0) {
        current_line = line_count - 1;
    }
    
    print("Line deleted\n");
}

// Show help
void textspace_show_help() {
    print("\n=== MPOP Text Editor Help ===\n");
    print("Commands:\n");
    print("  /run           - Execute the program\n");
    print("  /load <name>   - Load example (hello/fib/count/calc)\n");
    print("  /clear         - Clear all lines\n");
    print("  /del           - Delete current line\n");
    print("  /up            - Move cursor up\n");
    print("  /down          - Move cursor down\n");
    print("  /list          - Show program listing\n");
    print("  /help          - Show this help\n");
    print("  /exit          - Exit editor\n");
    print("\nMPOP Instructions:\n");
    print("  MOV R0, 42     - Move value to register\n");
    print("  ADD R0, R1     - Add registers\n");
    print("  PRINT R0       - Print register value\n");
    print("  PRINTC 65      - Print character (A)\n");
    print("  JMP label      - Jump to label\n");
    print("  HALT           - Stop program\n");

}

// Main textspace command function
void textspace_command(int argc, char* argv[]) {
    char userinput[COMMAND_BUFFER_SIZE];
    
    textspace_init();
    
    print("MPOP Text Editor - Type /help for commands\n");
    
    while (true) {
        textspace_display_program();
        
        if (keyboard_input(userinput) == -1) {
            break;
        }
        
        // Check for commands
        if (userinput[0] == '/') {
            if (strcmp(userinput, "/run") == 0) {
                // Execute program
                if (line_count == 0) {
                    print("No program to run!\n");
                    print("Press Enter to continue...");
                    char temp[2];
                    keyboard_input(temp);
                    continue;
                }
                
                print("Executing program...\n\n");
                
                // Get program string and run it
                char* program_str = textspace_get_program_string();
                
                // Initialize MPOP CPU
                mpop_cpu_t* cpu = mpop_init();
                if (!cpu) {
                    print("Error: Failed to initialize MPOP CPU\n");
                    print("Press Enter to continue...");
                    char temp[2];
                    keyboard_input(temp);
                    continue;
                }
                
                // Load and run program
                int result = mpop_load_program(cpu, program_str);
                if (result == MPOP_SUCCESS) {
                    print("Program loaded successfully\n");
                    result = mpop_run(cpu);
                    if (result == MPOP_SUCCESS) {
                        print("\nProgram completed successfully\n");
                    } else {
                        print("\nExecution error: ");
                        print(mpop_get_error_string(result));
                        print("\n");
                    }
                } else {
                    print("Failed to load program: ");
                    print(mpop_get_error_string(result));
                    print("\n");
                }
                
                mpop_cleanup(cpu);
                
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strncmp(userinput, "/load ", 6) == 0) {
                char temp[COMMAND_BUFFER_SIZE];
                keyboard_input(temp);
                textspace_load_example(temp);
                keyboard_input(temp);
                
                
            } else if (strcmp(userinput, "/clear") == 0) {
                textspace_init();
                print("Program cleared\n");
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/del") == 0) {
                textspace_delete_line();
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/up") == 0) {
                if (current_line > 0) {
                    current_line--;
                }
                
            } else if (strcmp(userinput, "/down") == 0) {
                if (current_line < line_count - 1) {
                    current_line++;
                }
                
            } else if (strcmp(userinput, "/list") == 0) {
                // Just refresh display
                continue;
                
            } else if (strcmp(userinput, "/help") == 0) {
                textspace_show_help();
                
            } else if (strcmp(userinput, "/exit") == 0) {
                if (program_modified) {
                    print("Program has unsaved changes. Exit anyway? (y/n): ");
                    char confirm[2];
                    keyboard_input(confirm);
                    if (confirm[0] != 'y' && confirm[0] != 'Y') {
                        continue;
                    }
                }
                break;
                
            } else {
                print("Unknown command: ");
                print(userinput);
                print("\nType /help for available commands\n");
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
            }
        } else {
            // Regular code input - add as new line
            if (strlen(userinput) > 0) {
                textspace_add_line(userinput);
                current_line = line_count - 1; // Move cursor to new line
            }
        }
    }
    
}

// Additional utility functions for the text editor

// Insert line at current position
void textspace_insert_line(const char* line) {
    if (line_count >= MAX_LINES - 1) {
        print("Maximum lines reached!\n");
        return;
    }
    
    // Shift lines down from current position
    for (int i = line_count; i > current_line; i--) {
        program_lines[i] = program_lines[i - 1];
    }
    
    // Find space in buffer for new line
    int buffer_pos = 0;
    for (int i = 0; i < line_count; i++) {
        if (program_lines[i] && i != current_line) {
            buffer_pos += strlen(program_lines[i]) + 1;
        }
    }
    
    // Check if we have enough space
    int line_len = strlen(line);
    if (buffer_pos + line_len + 1 >= MAX_PROGRAM_SIZE) {
        print("Program buffer full!\n");
        return;
    }
    
    // Copy line to buffer
    program_lines[current_line] = &program_buffer[buffer_pos];
    strcpy(program_lines[current_line], line);
    line_count++;
    program_modified = true;
}

// Edit current line
void textspace_edit_line(const char* new_content) {
    if (current_line >= 0 && current_line < line_count && program_lines[current_line]) {
        // Simple replacement - just overwrite if it fits
        if (strlen(new_content) <= strlen(program_lines[current_line])) {
            strcpy(program_lines[current_line], new_content);
            program_modified = true;
        } else {
            print("New line too long for current buffer space\n");
        }
    }
}

// Search for text in program
int textspace_search(const char* search_term) {
    for (int i = 0; i < line_count; i++) {
        if (program_lines[i] && strstr(program_lines[i], search_term)) {
            return i;
        }
    }
    return -1;
}

// Validate MPOP syntax
bool textspace_validate_syntax() {
    char* program_str = textspace_get_program_string();
    
    // Basic validation - check for common errors
    bool has_halt = false;
    int brace_count = 0;
    
    for (int i = 0; i < line_count; i++) {
        if (!program_lines[i]) continue;
        
        char* line = program_lines[i];
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == ';') continue;
        
        // Check for HALT instruction
        if (strstr(line, "HALT")) {
            has_halt = true;
        }
        
        // Check for balanced brackets (for memory addressing)
        for (int j = 0; line[j]; j++) {
            if (line[j] == '[') brace_count++;
            if (line[j] == ']') brace_count--;
        }
    }
    
    if (!has_halt) {
        print("Warning: Program doesn't contain HALT instruction\n");
    }
    
    if (brace_count != 0) {
        print("Warning: Unbalanced brackets in memory addressing\n");
    }
    
    return true; // Basic validation passed
}

// Export program to string format
void textspace_export_program() {
    print("\n=== Program Export ===\n");
    char* program_str = textspace_get_program_string();
    print("Program as single string:\n");
    print("\"");
    
    // Print with escaped newlines for easy copying
    for (int i = 0; program_str[i]; i++) {
        if (program_str[i] == '\n') {
            print("\\n");
        } else {
            char c[2] = {program_str[i], '\0'};
            print(c);
        }
    }
    print("\"\n\n");
    
    print("Program statistics:\n");
    print("Lines: ");
    print_integer(line_count);
    print("\n");
    print("Characters: ");
    print_integer(strlen(program_str));
    print("\n");
    
    print("Press Enter to continue...");
    char temp[2];
    keyboard_input(temp);
}

// Enhanced textspace command with more features
void textspace_advanced_command(int argc, char* argv[]) {
    char userinput[COMMAND_BUFFER_SIZE];
    bool debug_mode = false;
    
    textspace_init();
    
    print("MPOP Advanced Text Editor\n");
    print("Type /help for commands, /tutorial for MPOP tutorial\n");
    
    while (true) {
        textspace_display_program();
        
        if (debug_mode) {
            print("[DEBUG MODE] ");
        }
        
        if (keyboard_input(userinput) == -1) {
            print("\nExiting editor...\n");
            break;
        }
        
        // Enhanced command processing
        if (userinput[0] == '/') {
            if (strcmp(userinput, "/run") == 0) {
                if (line_count == 0) {
                    print("No program to run!\n");
                } else {print("  /save          - Save program (placeholder)\n");
print("  /save          - Save program (placeholder)\n");
                    print("Executing program...\n\n");
                    
                    char* program_str = textspace_get_program_string();
                    mpop_cpu_t* cpu = mpop_init();
                    
                    if (cpu) {
                        if (debug_mode) {
                            mpop_set_debug(cpu, true);
                        }
                        
                        int result = mpop_load_program(cpu, program_str);
                        if (result == MPOP_SUCCESS) {
                            print("Program loaded successfully\n");
                            result = mpop_run(cpu);
                            if (result == MPOP_SUCCESS) {
                                print("\nProgram completed successfully\n");
                            } else {
                                print("\nExecution error: ");
                                print(mpop_get_error_string(result));
                                print("\n");
                            }
                        } else {
                            print("Failed to load program: ");
                            print(mpop_get_error_string(result));
                            print("\n");
                        }
                        mpop_cleanup(cpu);
                    }
                }
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/debug") == 0) {
                debug_mode = !debug_mode;
                print("Debug mode ");
                print(debug_mode ? "enabled" : "disabled");
                print("\n");
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/validate") == 0) {
                print("Validating program syntax...\n");
                if (textspace_validate_syntax()) {
                    print("Syntax validation passed\n");
                } else {
                    print("Syntax validation failed\n");
                }
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/export") == 0) {
                textspace_export_program();
                
            } else if (strncmp(userinput, "/search ", 8) == 0) {
                char* search_term = userinput + 8;
                int found_line = textspace_search(search_term);
                if (found_line >= 0) {
                    current_line = found_line;
                    print("Found at line ");
                    print_integer(found_line + 1);
                    print("\n");
                } else {
                    print("Not found: ");
                    print(search_term);
                    print("\n");
                }
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/tutorial") == 0) {
                print("\n=== MPOP Programming Tutorial ===\n");
                print("1. Basic Instructions:\n");
                print("   MOV R0, 42      - Move value 42 to register R0\n");
                print("   ADD R0, R1      - Add R1 to R0, store in R0\n");
                print("   SUB R0, R1      - Subtract R1 from R0\n");
                print("   MUL R0, R1      - Multiply R0 by R1\n");
                print("   DIV R0, R1      - Divide R0 by R1\n\n");
                
                print("2. Input/Output:\n");
                print("   PRINT R0        - Print value in R0\n");
                print("   PRINTC 65       - Print character (65 = 'A')\n");
                print("   INPUT R0        - Get input from user\n\n");
                
                print("3. Control Flow:\n");
                print("   CMP R0, R1      - Compare R0 and R1\n");
                print("   JE label        - Jump if equal\n");
                print("   JNE label       - Jump if not equal\n");
                print("   JMP label       - Unconditional jump\n");
                print("   label:          - Define a label\n\n");
                
                print("4. Example Program:\n");
                print("   MOV R0, 5       ; Counter\n");
                print("   loop:\n");
                print("   PRINT R0        ; Print counter\n");
                print("   DEC R0          ; Decrement\n");
                print("   JNZ loop        ; Jump if not zero\n");
                print("   HALT            ; End program\n\n");
                
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strncmp(userinput, "/load ", 6) == 0) {
                char* example_name = userinput + 6;
                textspace_load_example(example_name);
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/clear") == 0) {
                textspace_init();
                print("Program cleared\n");
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/del") == 0) {
                textspace_delete_line();
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
                
            } else if (strcmp(userinput, "/up") == 0) {
                if (current_line > 0) current_line--;
                
            } else if (strcmp(userinput, "/down") == 0) {
                if (current_line < line_count - 1) current_line++;
                
            } else if (strcmp(userinput, "/help") == 0) {
                textspace_show_help();
                
            } else if (strcmp(userinput, "/exit") == 0) {
                if (program_modified) {
                    print("Program has unsaved changes. Exit anyway? (y/n): ");
                    char confirm[2];
                    keyboard_input(confirm);
                    if (confirm[0] != 'y' && confirm[0] != 'Y') {
                        continue;
                    }
                }
                break;
                
            } else {
                print("Unknown command: ");
                print(userinput);
                print("\nType /help for available commands\n");
                print("Press Enter to continue...");
                char temp[2];
                keyboard_input(temp);
            }
        } else {
            // Regular code input
            if (strlen(userinput) > 0) {
                textspace_add_line(userinput);
                current_line = line_count - 1;
            }
        }
    }
}