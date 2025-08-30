#include "../terminal/terminal.h"
#include "../errors/error.h"
#include "../keyboard/keyboard.h"
#include "../memory/memory.h"
#include "meow.h"
#define BF_MEMORY_SIZE 30000
#define BF_STACK_SIZE 1000
#define BF_INPUT_SIZE 1000


bf_state_t* bf_init() {
    if (bf_state_in_use) {
        print("Error: Brainfuck interpreter already in use\n");
        return NULL;
    }
    
    bf_state_in_use = 1;
    
    static_bf_state.memory = static_bf_memory;
    static_bf_state.loop_stack = static_bf_stack;
    static_bf_state.input_buffer = static_bf_input;
    
    // Initialize memory
    for (int i = 0; i < 1000; i++) {
        static_bf_memory[i] = 0;
    }
    
    static_bf_state.code_ptr = 0;
    static_bf_state.data_ptr = 0;
    static_bf_state.stack_ptr = 0;
    static_bf_state.input_ptr = 0;
    static_bf_state.input_length = 0;
    
    return &static_bf_state;
}
void bf_cleanup(bf_state_t* state) {
    bf_state_in_use = 0;
}
// Find matching bracket
int find_matching_bracket(char* code, int pos, int length, char open, char close) {
    int bracket_count = 1;
    int direction = (open == '[') ? 1 : -1;
    
    pos += direction;
    
    while (pos >= 0 && pos < length && bracket_count > 0) {
        if (code[pos] == open) {
            bracket_count++;
        } else if (code[pos] == close) {
            bracket_count--;
        }
        
        if (bracket_count > 0) {
            pos += direction;
        }
    }
    
    return (bracket_count == 0) ? pos : -1;
}

// Execute Brainfuck code
int bf_execute(bf_state_t* state) {
    while (state->code_ptr < state->code_length) {
        char instruction = state->code[state->code_ptr];
        
        switch (instruction) {
            case '>':
                state->data_ptr++;
                if (state->data_ptr >= BF_MEMORY_SIZE) {
                    print("Error: Memory pointer overflow\n");
                    return -1;
                }
                break;
                
            case '<':
                state->data_ptr--;
                if (state->data_ptr < 0) {
                    print("Error: Memory pointer underflow\n");
                    return -1;
                }
                break;
                
            case '+':
                state->memory[state->data_ptr]++;
                break;
                
            case '-':
                state->memory[state->data_ptr]--;
                break;
                
            case '.':
                // Output character
                char output[2] = {state->memory[state->data_ptr], '\0'};
                print(output);
                break;
                
            case ',':
                // Input character
                if (state->input_ptr < state->input_length) {
                    state->memory[state->data_ptr] = state->input_buffer[state->input_ptr];
                    state->input_ptr++;
                } else {
                    // Get more input
                    print("Input: ");
                    char temp_input[256];
                    keyboard_input(temp_input);
                    if (strlen(temp_input) > 0) {
                        state->memory[state->data_ptr] = temp_input[0];
                    } else {
                        state->memory[state->data_ptr] = 0;
                    }
                }
                break;
                
            case '[':
                if (state->memory[state->data_ptr] == 0) {
                    // Jump to matching ]
                    int match = find_matching_bracket(state->code, state->code_ptr, 
                                                    state->code_length, '[', ']');
                    if (match == -1) {
                        print("Error: Unmatched '['\n");
                        return -1;
                    }
                    state->code_ptr = match;
                } else {
                    // Push current position to stack
                    if (state->stack_ptr >= BF_STACK_SIZE) {
                        print("Error: Loop stack overflow\n");
                        return -1;
                    }
                    state->loop_stack[state->stack_ptr] = state->code_ptr;
                    state->stack_ptr++;
                }
                break;
                
            case ']':
                if (state->stack_ptr <= 0) {
                    print("Error: Unmatched ']'\n");
                    return -1;
                }
                
                if (state->memory[state->data_ptr] != 0) {
                    // Jump back to matching [
                    state->code_ptr = state->loop_stack[state->stack_ptr - 1];
                } else {
                    // Pop from stack
                    state->stack_ptr--;
                }
                break;
                
            default:
                // Ignore non-Brainfuck characters (comments)
                break;
        }
        
        state->code_ptr++;
    }
    
    return 0;
}

void brainfuck_command(int argc, char* argv[]) {
    char input[COMMAND_BUFFER_SIZE];
    
    print("\nBrainfuck Interpreter\n");
    print("Commands:\n");
    print("  run <code>  - Execute Brainfuck code\n");
    print("  hello       - Run Hello World example \n");
    print("  help        - Show this help\n");
    print("  exit        - Exit interpreter\n\n");
    
    while (true) {
        print("bf> ");
        keyboard_input(input);
        
        if (strcmp(input, "exit") == 0) {
            break;
        } else if (strcmp(input, "help") == 0) {
            print("Brainfuck commands:\n");
            print("  >  Move pointer right\n");
            print("  <  Move pointer left\n");
            print("  +  Increment cell\n");
            print("  -  Decrement cell\n");
            print("  .  Output cell as character\n");
            print("  ,  Input character to cell\n");
            print("  [  Start loop if cell != 0\n");
            print("  ]  End loop, jump back if cell != 0\n");
        } else if (strcmp(input, "hello") == 0) {
            // Hello World example
            char* hello_world = "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.";
            
            bf_state_t* state = bf_init();
            if (!state) {
                print("Error: Failed to initialize Brainfuck interpreter\n");
                continue;
            }
            
            state->code = hello_world;
            state->code_length = strlen(hello_world);
            
            print("Output: ");
            int result = bf_execute(state);
            print("\n");
            
            if (result != 0) {
                print("Execution failed\n");
            }
            
            bf_cleanup(state);
        } else if (strncmp(input, "run ", 4) == 0) {
            char* code = input + 4; // Skip "run "
            
            if (strlen(code) == 0) {
                print("Error: No code provided\n");
                continue;
            }
            
            bf_state_t* state = bf_init();
            if (!state) {
                print("Error: Failed to initialize Brainfuck interpreter\n");
                continue;
            }
            
            state->code = code;
            state->code_length = strlen(code);
            
            print("Output: ");
            int result = bf_execute(state);
            print("\n");
            
            if (result != 0) {
                print("Execution failed\n");
            }
            
            bf_cleanup(state);
        } else {
            print("Unknown command. Type 'help' for available commands.\n");
        }
    }
}

