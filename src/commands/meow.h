#ifndef BRAINFUCK_H
#define BRAINFUCK_H

#include <stdint.h>
#include <stdbool.h>

// Configuration constants
#define BF_MEMORY_SIZE 30000
#define BF_STACK_SIZE 1000
#define BF_INPUT_SIZE 1000

// Brainfuck interpreter state structure
typedef struct {
    char* code;              // Pointer to the Brainfuck code
    int code_ptr;            // Current position in code
    int code_length;         // Length of the code
    char* memory;            // Memory array (30,000 cells)
    int data_ptr;            // Current memory cell pointer
    int* loop_stack;         // Stack for handling loops
    int stack_ptr;           // Current stack position
    char* input_buffer;      // Buffer for input characters
    int input_ptr;           // Current position in input buffer
    int input_length;        // Length of input buffer
} bf_state_t;
static bf_state_t static_bf_state;
static char static_bf_memory[1000];
static int static_bf_stack[100];
static char static_bf_input[256];
static int bf_state_in_use = 0;
// Error codes
typedef enum {
    BF_SUCCESS = 0,
    BF_ERROR_MEMORY_OVERFLOW = -1,
    BF_ERROR_MEMORY_UNDERFLOW = -2,
    BF_ERROR_UNMATCHED_BRACKET = -3,
    BF_ERROR_STACK_OVERFLOW = -4,
    BF_ERROR_INIT_FAILED = -5
} bf_error_t;

// Function prototypes

/**
 * Initialize Brainfuck interpreter state
 * @return Pointer to initialized bf_state_t structure, or NULL on failure
 */
bf_state_t* bf_init(void);

/**
 * Clean up Brainfuck interpreter state and free memory
 * @param state Pointer to bf_state_t structure to clean up
 */
void bf_cleanup(bf_state_t* state);

/**
 * Find matching bracket in Brainfuck code
 * @param code Pointer to the code string
 * @param pos Current position in code
 * @param length Length of the code
 * @param open Opening bracket character ('[' or ']')
 * @param close Closing bracket character (']' or '[')
 * @return Position of matching bracket, or -1 if not found
 */
int find_matching_bracket(char* code, int pos, int length, char open, char close);

/**
 * Execute Brainfuck code
 * @param state Pointer to initialized bf_state_t structure
 * @return 0 on success, negative error code on failure
 */
int bf_execute(bf_state_t* state);

/**
 * Set input buffer for Brainfuck program
 * @param state Pointer to bf_state_t structure
 * @param input Input string to be used by ',' commands
 * @return 0 on success, negative error code on failure
 */
int bf_set_input(bf_state_t* state, const char* input);

/**
 * Reset Brainfuck interpreter state for new execution
 * @param state Pointer to bf_state_t structure
 */
void bf_reset(bf_state_t* state);

/**
 * Validate Brainfuck code for syntax errors
 * @param code Pointer to the code string
 * @param length Length of the code
 * @return 0 if valid, negative error code if invalid
 */
int bf_validate_code(const char* code, int length);

/**
 * Get error message string for error code
 * @param error_code Error code returned by BF functions
 * @return String description of the error
 */
const char* bf_get_error_string(int error_code);

/**
 * Main Brainfuck command interface
 * @param argc Number of arguments
 * @param argv Array of argument strings
 */
void brainfuck_command(int argc, char* argv[]);

/**
 * Execute Brainfuck code from string
 * @param code Brainfuck code to execute
 * @param input Optional input string (can be NULL)
 * @return 0 on success, negative error code on failure
 */
int bf_run_code(const char* code, const char* input);

/**
 * Print memory dump for debugging
 * @param state Pointer to bf_state_t structure
 * @param start Start position in memory
 * @param count Number of cells to display
 */
void bf_debug_memory(bf_state_t* state, int start, int count);

/**
 * Step through code execution (for debugging)
 * @param state Pointer to bf_state_t structure
 * @return 0 to continue, 1 to stop, negative on error
 */
int bf_step(bf_state_t* state);

// Built-in example programs
extern const char* BF_HELLO_WORLD;
extern const char* BF_ECHO;
extern const char* BF_ADD_TWO_NUMBERS;
extern const char* BF_FIBONACCI;

// Utility macros
#define BF_IS_VALID_INSTRUCTION(c) ((c) == '>' || (c) == '<' || (c) == '+' || \
                                    (c) == '-' || (c) == '.' || (c) == ',' || \
                                    (c) == '[' || (c) == ']')

#define BF_IS_LOOP_START(c) ((c) == '[')
#define BF_IS_LOOP_END(c) ((c) == ']')

#endif // BRAINFUCK_H

