// script.h - Enhanced Script Engine with Lexer/Parser
#ifndef SCRIPT_H
#define SCRIPT_H


#include <stdbool.h>

#define MAX_SCRIPT_VARIABLES 64
#define MAX_VARIABLE_NAME 32
#define MAX_VARIABLE_VALUE 256
#define MAX_SCRIPT_LINE_LENGTH 512
#define MAX_ARGUMENTS 32
#define MAX_SCRIPT_DEPTH 16

// Variable storage
typedef struct {
    char name[MAX_VARIABLE_NAME];
    char value[MAX_VARIABLE_VALUE];
    bool used;
} script_variable_t;

// Script execution context
typedef struct {
    script_variable_t variables[MAX_SCRIPT_VARIABLES];
    bool echo_mode;
    int last_exit_code;
    
    // Control flow state
    int block_depth;
    bool skip_execution[MAX_SCRIPT_DEPTH];
    bool in_loop[MAX_SCRIPT_DEPTH];
    int loop_start_pos[MAX_SCRIPT_DEPTH];
    
} script_context_t;

// Core functions
void script_init(void);
int script_execute_file(const char* filename);
int script_execute_string(const char* code);
void script_run_autoexec(void);

// Variable management
int script_set_variable(script_context_t* ctx, const char* name, const char* value);
const char* script_get_variable(script_context_t* ctx, const char* name);
void script_unset_variable(script_context_t* ctx, const char* name);

// Get global context
script_context_t* script_get_global_context(void);

#endif // SCRIPT_H