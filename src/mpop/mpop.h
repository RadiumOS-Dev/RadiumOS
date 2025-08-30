#ifndef MPOP_H
#define MPOP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>  // For va_list
// Configuration constants
#define MPOP_MEMORY_SIZE 1024
#define MPOP_STACK_SIZE 256
#define MPOP_REGISTER_COUNT 16
#define MPOP_MAX_CODE_SIZE 2048
#define MPOP_MAX_LABELS 64
#define MPOP_MAX_LABEL_NAME 32

// MPOP Opcodes
typedef enum {
    // Data movement
    MPOP_NOP = 0x00,        // No operation
    MPOP_MOV = 0x01,        // MOV dst, src - Move data
    MPOP_LOAD = 0x02,       // LOAD reg, addr - Load from memory
    MPOP_STORE = 0x03,      // STORE addr, reg - Store to memory
    
    // Arithmetic
    MPOP_ADD = 0x10,        // ADD dst, src - Addition
    MPOP_SUB = 0x11,        // SUB dst, src - Subtraction
    MPOP_MUL = 0x12,        // MUL dst, src - Multiplication
    MPOP_DIV = 0x13,        // DIV dst, src - Division
    MPOP_MOD = 0x14,        // MOD dst, src - Modulo
    MPOP_INC = 0x15,        // INC reg - Increment
    MPOP_DEC = 0x16,        // DEC reg - Decrement
    
    // Logical
    MPOP_AND = 0x20,        // AND dst, src - Bitwise AND
    MPOP_OR = 0x21,         // OR dst, src - Bitwise OR
    MPOP_XOR = 0x22,        // XOR dst, src - Bitwise XOR
    MPOP_NOT = 0x23,        // NOT reg - Bitwise NOT
    MPOP_SHL = 0x24,        // SHL reg, count - Shift left
    MPOP_SHR = 0x25,        // SHR reg, count - Shift right
    
    // Comparison
    MPOP_CMP = 0x30,        // CMP a, b - Compare
    MPOP_TEST = 0x31,       // TEST reg, mask - Test bits
    
    // Control flow
    MPOP_JMP = 0x40,        // JMP addr - Unconditional jump
    MPOP_JZ = 0x41,         // JZ addr - Jump if zero
    MPOP_JNZ = 0x42,        // JNZ addr - Jump if not zero
    MPOP_JE = 0x43,         // JE addr - Jump if equal
    MPOP_JNE = 0x44,        // JNE addr - Jump if not equal
    MPOP_JL = 0x45,         // JL addr - Jump if less
    MPOP_JG = 0x46,         // JG addr - Jump if greater
    MPOP_CALL = 0x47,       // CALL addr - Call subroutine
    MPOP_RET = 0x48,        // RET - Return from subroutine
    
    // Stack operations
    MPOP_PUSH = 0x50,       // PUSH src - Push to stack
    MPOP_POP = 0x51,        // POP dst - Pop from stack
    
    // I/O operations
    MPOP_PRINT = 0x60,      // PRINT reg - Print register value
    MPOP_PRINTC = 0x61,     // PRINTC reg - Print as character
    MPOP_PRINTS = 0x62,     // PRINTS addr - Print string at address
    MPOP_INPUT = 0x63,      // INPUT reg - Get input to register
    
    // System operations
    MPOP_HALT = 0xFF        // HALT - Stop execution
} mpop_opcode_t;

// Operand types
typedef enum {
    MPOP_OPERAND_REGISTER,  // Register (R0-R15)
    MPOP_OPERAND_IMMEDIATE, // Immediate value
    MPOP_OPERAND_MEMORY,    // Memory address
    MPOP_OPERAND_LABEL      // Label reference
} mpop_operand_type_t;

// Operand structure
typedef struct {
    mpop_operand_type_t type;
    union {
        int reg_num;        // Register number (0-15)
        int32_t immediate;  // Immediate value
        uint32_t address;   // Memory address
        char label[MPOP_MAX_LABEL_NAME]; // Label name
    } value;
} mpop_operand_t;

// Instruction structure
typedef struct {
    mpop_opcode_t opcode;
    mpop_operand_t operand1;
    mpop_operand_t operand2;
} mpop_instruction_t;

// Label structure
typedef struct {
    char name[MPOP_MAX_LABEL_NAME];
    uint32_t address;
} mpop_label_t;

// CPU state structure
typedef struct {
    int32_t registers[MPOP_REGISTER_COUNT]; // R0-R15
    uint8_t memory[MPOP_MEMORY_SIZE];       // Memory space
    int32_t stack[MPOP_STACK_SIZE];         // Stack
    uint32_t pc;                            // Program counter
    uint32_t sp;                            // Stack pointer
    
    // Flags
    bool zero_flag;
    bool carry_flag;
    bool negative_flag;
    
    // Program storage
    mpop_instruction_t program[MPOP_MAX_CODE_SIZE];
    uint32_t program_size;
    
    // Labels
    mpop_label_t labels[MPOP_MAX_LABELS];
    uint32_t label_count;
    
    // Execution state
    bool running;
    bool debug_mode;
} mpop_cpu_t;

// Error codes
typedef enum {
    MPOP_SUCCESS = 0,
    MPOP_ERROR_INVALID_OPCODE = -1,
    MPOP_ERROR_INVALID_REGISTER = -2,
    MPOP_ERROR_INVALID_ADDRESS = -3,
    MPOP_ERROR_STACK_OVERFLOW = -4,
    MPOP_ERROR_STACK_UNDERFLOW = -5,
    MPOP_ERROR_DIVISION_BY_ZERO = -6,
    MPOP_ERROR_LABEL_NOT_FOUND = -7,
    MPOP_ERROR_PROGRAM_TOO_LARGE = -8,
    MPOP_ERROR_PARSE_ERROR = -9
} mpop_error_t;

// Function prototypes

/**
 * Initialize MPOP CPU
 * @return Pointer to initialized CPU state
 */
mpop_cpu_t* mpop_init(void);

/**
 * Clean up MPOP CPU
 * @param cpu Pointer to CPU state
 */
void mpop_cleanup(mpop_cpu_t* cpu);

/**
 * Reset CPU state
 * @param cpu Pointer to CPU state
 */
void mpop_reset(mpop_cpu_t* cpu);

/**
 * Load program from assembly text
 * @param cpu Pointer to CPU state
 * @param assembly Assembly code string
 * @return Error code
 */
int mpop_load_program(mpop_cpu_t* cpu, const char* assembly);

/**
 * Execute single instruction
 * @param cpu Pointer to CPU state
 * @return Error code
 */
int mpop_step(mpop_cpu_t* cpu);

/**
 * Execute program until halt or error
 * @param cpu Pointer to CPU state
 * @return Error code
 */
int mpop_run(mpop_cpu_t* cpu);

/**
 * Set debug mode
 * @param cpu Pointer to CPU state
 * @param debug Enable/disable debug mode
 */
void mpop_set_debug(mpop_cpu_t* cpu, bool debug);

/**
 * Print CPU state for debugging
 * @param cpu Pointer to CPU state
 */
void mpop_debug_print(mpop_cpu_t* cpu);

/**
 * Get error message string
 * @param error Error code
 * @return Error message string
 */
const char* mpop_get_error_string(int error);

/**
 * Parse operand from string
 * @param str Operand string
 * @param operand Output operand structure
 * @return Error code
 */
int mpop_parse_operand(const char* str, mpop_operand_t* operand);

/**
 * Resolve label to address
 * @param cpu Pointer to CPU state
 * @param label_name Label name
 * @return Address or -1 if not found
 */
int mpop_resolve_label(mpop_cpu_t* cpu, const char* label_name);

/**
 * Add label to CPU
 * @param cpu Pointer to CPU state
 * @param name Label name
 * @param address Label address
 * @return Error code
 */
int mpop_add_label(mpop_cpu_t* cpu, const char* name, uint32_t address);

/**
 * MPOP command interface
 * @param argc Number of arguments
 * @param argv Array of argument strings
 */
void mpop_command(int argc, char* argv[]);

// Built-in example programs
extern const char* MPOP_HELLO_WORLD;
extern const char* MPOP_FIBONACCI;
extern const char* MPOP_FACTORIAL;
extern const char* MPOP_BUBBLE_SORT;

// Utility macros
#define MPOP_IS_VALID_REGISTER(r) ((r) >= 0 && (r) < MPOP_REGISTER_COUNT)
#define MPOP_IS_VALID_ADDRESS(a) ((a) < MPOP_MEMORY_SIZE)

#endif // MPOP_H
