#include "mpop.h"
#include "../terminal/terminal.h"
#include "../errors/error.h"
#include "../keyboard/keyboard.h"
#include "../memory/memory.h"
#include "../utility/utility.h"

const char* MPOP_TESTr =
    "; MPOP Test Suite - Comprehensive CPU Test\n"
    "; Tests arithmetic, logic, jumps, stack, and I/O\n"
    "\n"
    "; === Test 1: Basic Arithmetic ===\n"
    "MOV R0, 10\n"
    "MOV R1, 5\n"
    "ADD R2, R0, R1\n"      // R2 = 15
    "SUB R3, R0, R1\n"      // R3 = 5
    "MUL R4, R0, R1\n"      // R4 = 50
    "DIV R5, R0, R1\n"      // R5 = 2
    "MOD R6, R0, R1\n"      // R6 = 0
    "\n"
    "; Print arithmetic results\n"
    "PRINTC 65\n"           // 'A'
    "PRINTC 114\n"          // 'r'
    "PRINTC 105\n"          // 'i'
    "PRINTC 116\n"          // 't'
    "PRINTC 104\n"          // 'h'
    "PRINTC 58\n"           // ':' 
    "PRINTC 32\n"           // ' '
    "PRINT R2\n"            // 15
    "PRINTC 32\n"           // ' '
    "PRINT R3\n"            // 5
    "PRINTC 32\n"           // ' '
    "PRINT R4\n"            // 50
    "PRINTC 10\n"           // newline
    "\n"
    "; === Test 2: Logic Operations ===\n"
    "MOV R0, 15\n"          // 1111 in binary
    "MOV R1, 10\n"          // 1010 in binary
    "AND R7, R0, R1\n"      // R7 = 10 (1010)
    "OR R8, R0, R1\n"       // R8 = 15 (1111)
    "XOR R9, R0, R1\n"      // R9 = 5  (0101)
    "\n"
    "; Print logic results\n"
    "PRINTC 76\n"           // 'L'
    "PRINTC 111\n"          // 'o'
    "PRINTC 103\n"          // 'g'
    "PRINTC 105\n"          // 'i'
    "PRINTC 99\n"           // 'c'
    "PRINTC 58\n"           // ':'
    "PRINTC 32\n"           // ' '
    "PRINT R7\n"            // 10
    "PRINTC 32\n"           // ' '
    "PRINT R8\n"            // 15
    "PRINTC 32\n"           // ' '
    "PRINT R9\n"            // 5
    "PRINTC 10\n"           // newline
    "\n"
    "; === Test 3: Stack Operations ===\n"
    "MOV R0, 100\n"
    "MOV R1, 200\n"
    "MOV R2, 300\n"
    "PUSH R0\n"
    "PUSH R1\n"
    "PUSH R2\n"
    "\n"
    "; Pop in reverse order\n"
    "POP R10\n"             // R10 = 300
    "POP R11\n"             // R11 = 200
    "POP R12\n"             // R12 = 100
    "\n"
    "; Print stack test results\n"
    "PRINTC 83\n"           // 'S'
    "PRINTC 116\n"          // 't'
    "PRINTC 97\n"           // 'a'
    "PRINTC 99\n"           // 'c'
    "PRINTC 107\n"          // 'k'
    "PRINTC 58\n"           // ':'
    "PRINTC 32\n"           // ' '
    "PRINT R10\n"           // 300
    "PRINTC 32\n"           // ' '
    "PRINT R11\n"           // 200
    "PRINTC 32\n"           // ' '
    "PRINT R12\n"           // 100
    "PRINTC 10\n"           // newline
    "\n"
    "; === Test 4: Conditional Jumps ===\n"
    "MOV R0, 5\n"
    "MOV R1, 5\n"
    "CMP R0, R1\n"
    "JE equal_test\n"
    "PRINTC 70\n"           // 'F' (should not print)
    "JMP after_equal\n"
    "equal_test:\n"
    "PRINTC 84\n"           // 'T' (should print)
    "after_equal:\n"
    "\n"
    "; Test not equal\n"
    "MOV R0, 3\n"
    "MOV R1, 7\n"
    "CMP R0, R1\n"
    "JNE not_equal_test\n"
    "PRINTC 70\n"           // 'F' (should not print)
    "JMP after_not_equal\n"
    "not_equal_test:\n"
    "PRINTC 84\n"           // 'T' (should print)
    "after_not_equal:\n"
    "PRINTC 10\n"           // newline
    "\n"
    "; === Test 5: Loop Test (Count down) ===\n"
    "MOV R0, 5\n"           // Counter
    "PRINTC 67\n"           // 'C'
    "PRINTC 111\n"          // 'o'
    "PRINTC 117\n"          // 'u'
    "PRINTC 110\n"          // 'n'
    "PRINTC 116\n"          // 't'
    "PRINTC 58\n"           // ':'
    "PRINTC 32\n"           // ' '
    "countdown_loop:\n"
    "PRINT R0\n"
    "PRINTC 32\n"           // ' '
    "DEC R0\n"
    "CMP R0, 0\n"
    "JNZ countdown_loop\n"
    "PRINTC 10\n"           // newline
    "\n"
    "; === Test 6: Function Call Test ===\n"
    "MOV R0, 42\n"
    "CALL print_function\n"
    "JMP test_complete\n"
    "\n"
    "print_function:\n"
    "PRINTC 70\n"           // 'F'
    "PRINTC 117\n"          // 'u'
    "PRINTC 110\n"          // 'n'
    "PRINTC 99\n"           // 'c'
    "PRINTC 58\n"           // ':'
    "PRINTC 32\n"           // ' '
    "PRINT R0\n"            // Print the value in R0
    "PRINTC 10\n"           // newline
    "RET\n"
    "\n"
    "test_complete:\n"
    "; === Test Summary ===\n"
    "PRINTC 84\n"           // 'T'
    "PRINTC 101\n"          // 'e'
    "PRINTC 115\n"          // 's'
    "PRINTC 116\n"          // 't'
    "PRINTC 32\n"           // ' '
    "PRINTC 67\n"           // 'C'
    "PRINTC 111\n"          // 'o'
    "PRINTC 109\n"          // 'm'
    "PRINTC 112\n"          // 'p'
    "PRINTC 108\n"          // 'l'
    "PRINTC 101\n"          // 'e'
    "PRINTC 116\n"          // 't'
    "PRINTC 101\n"          // 'e'
    "PRINTC 33\n"           // '!'
    "PRINTC 10\n"           // newline
    "HALT\n";


// Built-in example programs
const char* MPOP_HELLO_WORLD = 
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

const char* MPOP_FIBONACCI = 
    "MOV R0, 0\n"       // First fibonacci number
    "MOV R1, 1\n"       // Second fibonacci number
    "MOV R2, 10\n"      // Counter
    "loop:\n"
    "PRINT R0\n"        // printr current number
    "ADD R3, R0, R1\n"  // R3 = R0 + R1
    "MOV R0, R1\n"      // R0 = R1
    "MOV R1, R3\n"      // R1 = R3
    "DEC R2\n"          // Decrement counter
    "JNZ loop\n"        // Jump if not zero
    "HALT\n";

const char* MPOP_FACTORIAL = 
    "MOV R0, 5\n"       // Calculate 5!
    "MOV R1, 1\n"       // Result
    "loop:\n"
    "CMP R0, 0\n"
    "JE done\n"
    "MUL R1, R0\n"      // R1 = R1 * R0
    "DEC R0\n"
    "JMP loop\n"
    "done:\n"
    "PRINT R1\n"        // printr result
    "HALT\n";

// Static CPU instance for simplicity
static mpop_cpu_t static_cpu;
static bool cpu_initialized = false;

mpop_cpu_t* mpop_init(void) {
    if (cpu_initialized) {
        mpop_reset(&static_cpu);
        return &static_cpu;
    }
    
    // Initialize CPU state
    for (int i = 0; i < MPOP_REGISTER_COUNT; i++) {
        static_cpu.registers[i] = 0;
    }
    
    for (int i = 0; i < MPOP_MEMORY_SIZE; i++) {
        static_cpu.memory[i] = 0;
    }
    
    for (int i = 0; i < MPOP_STACK_SIZE; i++) {
        static_cpu.stack[i] = 0;
    }
    
    static_cpu.pc = 0;
    static_cpu.sp = 0;
    static_cpu.zero_flag = false;
    static_cpu.carry_flag = false;
    static_cpu.negative_flag = false;
    static_cpu.program_size = 0;
    static_cpu.label_count = 0;
    static_cpu.running = false;
    static_cpu.debug_mode = false;
    
    cpu_initialized = true;
    return &static_cpu;
}

void mpop_cleanup(mpop_cpu_t* cpu) {
    // Nothing to clean up with static allocation
}

void mpop_reset(mpop_cpu_t* cpu) {
    if (!cpu) return;
    
    for (int i = 0; i < MPOP_REGISTER_COUNT; i++) {
        cpu->registers[i] = 0;
    }
    
    cpu->pc = 0;
    cpu->sp = 0;
    cpu->zero_flag = false;
    cpu->carry_flag = false;
    cpu->negative_flag = false;
    cpu->running = false;
}

const char* mpop_get_error_string(int error) {
    switch (error) {
        case MPOP_SUCCESS: return "Success";
        case MPOP_ERROR_INVALID_OPCODE: return "Invalid opcode";
        case MPOP_ERROR_INVALID_REGISTER: return "Invalid register";
        case MPOP_ERROR_INVALID_ADDRESS: return "Invalid address";
        case MPOP_ERROR_STACK_OVERFLOW: return "Stack overflow";
        case MPOP_ERROR_STACK_UNDERFLOW: return "Stack underflow";
        case MPOP_ERROR_DIVISION_BY_ZERO: return "Division by zero";
        case MPOP_ERROR_LABEL_NOT_FOUND: return "Label not found";
        case MPOP_ERROR_PROGRAM_TOO_LARGE: return "Program too large";
                case MPOP_ERROR_PARSE_ERROR: return "Parse error";
        default: return "Unknown error";
    }
}

int mpop_parse_operand(const char* str, mpop_operand_t* operand) {
    if (!str || !operand) return MPOP_ERROR_PARSE_ERROR;
    
    // Skip whitespace
    while (*str == ' ' || *str == '\t') str++;
    
    if (*str == 'R' || *str == 'r') {
        // Register operand (R0, R1, etc.)
        str++;
        int reg_num = 0;
        while (*str >= '0' && *str <= '9') {
            reg_num = reg_num * 10 + (*str - '0');
            str++;
        }
        
        if (!MPOP_IS_VALID_REGISTER(reg_num)) {
            return MPOP_ERROR_INVALID_REGISTER;
        }
        
        operand->type = MPOP_OPERAND_REGISTER;
        operand->value.reg_num = reg_num;
        return MPOP_SUCCESS;
    } else if (*str >= '0' && *str <= '9') {
        // Immediate value
        int32_t value = 0;
        bool negative = false;
        
        if (*str == '-') {
            negative = true;
            str++;
        }
        
        while (*str >= '0' && *str <= '9') {
            value = value * 10 + (*str - '0');
            str++;
        }
        
        if (negative) value = -value;
        
        operand->type = MPOP_OPERAND_IMMEDIATE;
        operand->value.immediate = value;
        return MPOP_SUCCESS;
    } else if (*str == '[') {
        // Memory address [123]
        str++; // Skip '['
        uint32_t address = 0;
        
        while (*str >= '0' && *str <= '9') {
            address = address * 10 + (*str - '0');
            str++;
        }
        
        if (*str != ']') return MPOP_ERROR_PARSE_ERROR;
        
        if (!MPOP_IS_VALID_ADDRESS(address)) {
            return MPOP_ERROR_INVALID_ADDRESS;
        }
        
        operand->type = MPOP_OPERAND_MEMORY;
        operand->value.address = address;
        return MPOP_SUCCESS;
    } else {
        // Label
        int i = 0;
        while (*str && *str != ' ' && *str != '\t' && *str != '\n' && 
               *str != ',' && i < MPOP_MAX_LABEL_NAME - 1) {
            operand->value.label[i++] = *str++;
        }
        operand->value.label[i] = '\0';
        
        operand->type = MPOP_OPERAND_LABEL;
        return MPOP_SUCCESS;
    }
}

int mpop_add_label(mpop_cpu_t* cpu, const char* name, uint32_t address) {
    if (!cpu || !name || cpu->label_count >= MPOP_MAX_LABELS) {
        return MPOP_ERROR_PROGRAM_TOO_LARGE;
    }
    
    // Copy label name
    int i = 0;
    while (name[i] && i < MPOP_MAX_LABEL_NAME - 1) {
        cpu->labels[cpu->label_count].name[i] = name[i];
        i++;
    }
    cpu->labels[cpu->label_count].name[i] = '\0';
    cpu->labels[cpu->label_count].address = address;
    cpu->label_count++;
    
    return MPOP_SUCCESS;
}

int mpop_resolve_label(mpop_cpu_t* cpu, const char* label_name) {
    if (!cpu || !label_name) return -1;
    
    for (uint32_t i = 0; i < cpu->label_count; i++) {
        if (strcmp(cpu->labels[i].name, label_name) == 0) {
            return cpu->labels[i].address;
        }
    }
    
    return -1;
}

int mpop_load_program(mpop_cpu_t* cpu, const char* assembly) {
    if (!cpu || !assembly) return MPOP_ERROR_PARSE_ERROR;
    
    cpu->program_size = 0;
    cpu->label_count = 0;
    
    char line[256];
    int line_pos = 0;
    int asm_pos = 0;
    uint32_t instruction_count = 0;
    
    // First pass: collect labels
    while (assembly[asm_pos]) {
        // Read line
        line_pos = 0;
        while (assembly[asm_pos] && assembly[asm_pos] != '\n' && line_pos < 255) {
            line[line_pos++] = assembly[asm_pos++];
        }
        line[line_pos] = '\0';
        
        if (assembly[asm_pos] == '\n') asm_pos++;
        
        // Skip empty lines and comments
        if (line_pos == 0 || line[0] == ';') continue;
        
        // Check for label (ends with ':')
        if (line[line_pos - 1] == ':') {
            line[line_pos - 1] = '\0'; // Remove ':'
            mpop_add_label(cpu, line, instruction_count);
        } else {
            instruction_count++;
        }
    }
    
    // Second pass: parse instructions
    asm_pos = 0;
    instruction_count = 0;
    
    while (assembly[asm_pos] && instruction_count < MPOP_MAX_CODE_SIZE) {
        // Read line
        line_pos = 0;
        while (assembly[asm_pos] && assembly[asm_pos] != '\n' && line_pos < 255) {
            line[line_pos++] = assembly[asm_pos++];
        }
        line[line_pos] = '\0';
        
        if (assembly[asm_pos] == '\n') asm_pos++;
        
        // Skip empty lines, comments, and labels
        if (line_pos == 0 || line[0] == ';' || line[line_pos - 1] == ':') continue;
        
        // Parse instruction
        char* token = line;
        
        // Skip whitespace
        while (*token == ' ' || *token == '\t') token++;
        
        // Get opcode
        char opcode_str[16];
        int i = 0;
        while (*token && *token != ' ' && *token != '\t' && i < 15) {
            opcode_str[i++] = *token++;
        }
        opcode_str[i] = '\0';
        
        // Convert opcode string to enum
        mpop_opcode_t opcode = MPOP_NOP;
        if (strcmp(opcode_str, "NOP") == 0) opcode = MPOP_NOP;
        else if (strcmp(opcode_str, "MOV") == 0) opcode = MPOP_MOV;
        else if (strcmp(opcode_str, "LOAD") == 0) opcode = MPOP_LOAD;
        else if (strcmp(opcode_str, "STORE") == 0) opcode = MPOP_STORE;
        else if (strcmp(opcode_str, "ADD") == 0) opcode = MPOP_ADD;
        else if (strcmp(opcode_str, "SUB") == 0) opcode = MPOP_SUB;
        else if (strcmp(opcode_str, "MUL") == 0) opcode = MPOP_MUL;
        else if (strcmp(opcode_str, "DIV") == 0) opcode = MPOP_DIV;
        else if (strcmp(opcode_str, "MOD") == 0) opcode = MPOP_MOD;
        else if (strcmp(opcode_str, "INC") == 0) opcode = MPOP_INC;
        else if (strcmp(opcode_str, "DEC") == 0) opcode = MPOP_DEC;
        else if (strcmp(opcode_str, "AND") == 0) opcode = MPOP_AND;
        else if (strcmp(opcode_str, "OR") == 0) opcode = MPOP_OR;
        else if (strcmp(opcode_str, "XOR") == 0) opcode = MPOP_XOR;
        else if (strcmp(opcode_str, "NOT") == 0) opcode = MPOP_NOT;
        else if (strcmp(opcode_str, "SHL") == 0) opcode = MPOP_SHL;
        else if (strcmp(opcode_str, "SHR") == 0) opcode = MPOP_SHR;
        else if (strcmp(opcode_str, "CMP") == 0) opcode = MPOP_CMP;
        else if (strcmp(opcode_str, "TEST") == 0) opcode = MPOP_TEST;
        else if (strcmp(opcode_str, "JMP") == 0) opcode = MPOP_JMP;
        else if (strcmp(opcode_str, "JZ") == 0) opcode = MPOP_JZ;
        else if (strcmp(opcode_str, "JNZ") == 0) opcode = MPOP_JNZ;
        else if (strcmp(opcode_str, "JE") == 0) opcode = MPOP_JE;
        else if (strcmp(opcode_str, "JNE") == 0) opcode = MPOP_JNE;
        else if (strcmp(opcode_str, "JL") == 0) opcode = MPOP_JL;
        else if (strcmp(opcode_str, "JG") == 0) opcode = MPOP_JG;
        else if (strcmp(opcode_str, "CALL") == 0) opcode = MPOP_CALL;
        else if (strcmp(opcode_str, "RET") == 0) opcode = MPOP_RET;
        else if (strcmp(opcode_str, "PUSH") == 0) opcode = MPOP_PUSH;
        else if (strcmp(opcode_str, "POP") == 0) opcode = MPOP_POP;
        else if (strcmp(opcode_str, "PRINT") == 0) opcode = MPOP_PRINT;
        else if (strcmp(opcode_str, "PRINTC") == 0) opcode = MPOP_PRINTC;
        else if (strcmp(opcode_str, "PRINTS") == 0) opcode = MPOP_PRINTS;
        else if (strcmp(opcode_str, "INPUT") == 0) opcode = MPOP_INPUT;
        else if (strcmp(opcode_str, "HALT") == 0) opcode = MPOP_HALT;
        else return MPOP_ERROR_INVALID_OPCODE;
        
        cpu->program[instruction_count].opcode = opcode;
        
        // Parse operands
        while (*token == ' ' || *token == '\t') token++;
        
        if (*token) {
            // First operand
            char operand1_str[64];
            i = 0;
            while (*token && *token != ',' && *token != ' ' && *token != '\t' && i < 63) {
                operand1_str[i++] = *token++;
            }
            operand1_str[i] = '\0';
            
            int result = mpop_parse_operand(operand1_str, &cpu->program[instruction_count].operand1);
            if (result != MPOP_SUCCESS) return result;
            
            // Skip comma and whitespace
            while (*token == ',' || *token == ' ' || *token == '\t') token++;
            
            if (*token) {
                // Second operand
                char operand2_str[64];
                i = 0;
                while (*token && *token != ' ' && *token != '\t' && i < 63) {
                    operand2_str[i++] = *token++;
                }
                operand2_str[i] = '\0';
                
                result = mpop_parse_operand(operand2_str, &cpu->program[instruction_count].operand2);
                if (result != MPOP_SUCCESS) return result;
            }
        }
        
        instruction_count++;
    }
    
    cpu->program_size = instruction_count;
    return MPOP_SUCCESS;
}

int32_t mpop_get_operand_value(mpop_cpu_t* cpu, mpop_operand_t* operand) {
    switch (operand->type) {
        case MPOP_OPERAND_REGISTER:
            return cpu->registers[operand->value.reg_num];
        case MPOP_OPERAND_IMMEDIATE:
            return operand->value.immediate;
        case MPOP_OPERAND_MEMORY:
            return cpu->memory[operand->value.address];
        case MPOP_OPERAND_LABEL:
            return mpop_resolve_label(cpu, operand->value.label);
        default:
            return 0;
    }
}

void mpop_set_operand_value(mpop_cpu_t* cpu, mpop_operand_t* operand, int32_t value) {
    switch (operand->type) {
        case MPOP_OPERAND_REGISTER:
            cpu->registers[operand->value.reg_num] = value;
            break;
        case MPOP_OPERAND_MEMORY:
            cpu->memory[operand->value.address] = (uint8_t)value;
            break;
        default:
            break;
    }
}

void mpop_update_flags(mpop_cpu_t* cpu, int32_t result) {
    cpu->zero_flag = (result == 0);
    cpu->negative_flag = (result < 0);
    cpu->carry_flag = false; // Simplified for now
}

int mpop_step(mpop_cpu_t* cpu) {
    if (!cpu || cpu->pc >= cpu->program_size) {
        cpu->running = false;
        return MPOP_SUCCESS;
    }
    
    mpop_instruction_t* instr = &cpu->program[cpu->pc];
    int32_t val1, val2, result;
    
    if (cpu->debug_mode) {
        printr("PC: %d, Opcode: %d\n", cpu->pc, instr->opcode);
    }
    
    switch (instr->opcode) {
        case MPOP_NOP:
            break;
            
        case MPOP_MOV:
            val1 = mpop_get_operand_value(cpu, &instr->operand2);
            mpop_set_operand_value(cpu, &instr->operand1, val1);
            break;
            
        case MPOP_ADD:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 + val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_SUB:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 - val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
                        mpop_update_flags(cpu, result);
            break;
            
        case MPOP_MUL:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 * val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_DIV:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            if (val2 == 0) return MPOP_ERROR_DIVISION_BY_ZERO;
            result = val1 / val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_MOD:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            if (val2 == 0) return MPOP_ERROR_DIVISION_BY_ZERO;
            result = val1 % val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_INC:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            result = val1 + 1;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_DEC:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            result = val1 - 1;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_AND:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 & val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_OR:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 | val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_XOR:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 ^ val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_NOT:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            result = ~val1;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_SHL:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 << val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_SHR:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 >> val2;
            mpop_set_operand_value(cpu, &instr->operand1, result);
            mpop_update_flags(cpu, result);
            break;
            
        case MPOP_CMP:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            val2 = mpop_get_operand_value(cpu, &instr->operand2);
            result = val1 - val2;
            mpop_update_flags(cpu, result);
            break;
            
            
        case MPOP_JMP:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            cpu->pc = val1;
            return MPOP_SUCCESS; // Don't increment PC
            
        case MPOP_JZ:
            if (cpu->zero_flag) {
                val1 = mpop_get_operand_value(cpu, &instr->operand1);
                cpu->pc = val1;
                return MPOP_SUCCESS;
            }
            break;
            
        case MPOP_JNZ:
            if (!cpu->zero_flag) {
                val1 = mpop_get_operand_value(cpu, &instr->operand1);
                cpu->pc = val1;
                return MPOP_SUCCESS;
            }
            break;
            
        case MPOP_JE:
            if (cpu->zero_flag) {
                val1 = mpop_get_operand_value(cpu, &instr->operand1);
                cpu->pc = val1;
                return MPOP_SUCCESS;
            }
            break;
            
        case MPOP_JNE:
            if (!cpu->zero_flag) {
                val1 = mpop_get_operand_value(cpu, &instr->operand1);
                cpu->pc = val1;
                return MPOP_SUCCESS;
            }
            break;
            
        case MPOP_JL:
            if (cpu->negative_flag) {
                val1 = mpop_get_operand_value(cpu, &instr->operand1);
                cpu->pc = val1;
                return MPOP_SUCCESS;
            }
            break;
            
        case MPOP_JG:
            if (!cpu->negative_flag && !cpu->zero_flag) {
                val1 = mpop_get_operand_value(cpu, &instr->operand1);
                cpu->pc = val1;
                return MPOP_SUCCESS;
            }
            break;
            
        case MPOP_CALL:
            if (cpu->sp >= MPOP_STACK_SIZE) return MPOP_ERROR_STACK_OVERFLOW;
            cpu->stack[cpu->sp++] = cpu->pc + 1;
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            cpu->pc = val1;
            return MPOP_SUCCESS;
            
        case MPOP_RET:
            if (cpu->sp <= 0) return MPOP_ERROR_STACK_UNDERFLOW;
            cpu->pc = cpu->stack[--cpu->sp];
            return MPOP_SUCCESS;
            
        case MPOP_PUSH:
            if (cpu->sp >= MPOP_STACK_SIZE) return MPOP_ERROR_STACK_OVERFLOW;
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            cpu->stack[cpu->sp++] = val1;
            break;
            
        case MPOP_POP:
            if (cpu->sp <= 0) return MPOP_ERROR_STACK_UNDERFLOW;
            val1 = cpu->stack[--cpu->sp];
            mpop_set_operand_value(cpu, &instr->operand1, val1);
            break;
            
        case MPOP_PRINT:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            printr("%d ", val1);
            break;
            
        case MPOP_PRINTC:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            {
                char output[2] = {(char)val1, '\0'};
                printr(output);
            }
            break;
            
        case MPOP_PRINTS:
            val1 = mpop_get_operand_value(cpu, &instr->operand1);
            if (MPOP_IS_VALID_ADDRESS(val1)) {
                char* str = (char*)&cpu->memory[val1];
                printr(str);
            }
            break;
            
        case MPOP_INPUT:
            {
                char input[256];
                printr("Input: ");
                keyboard_input(input);
                int value = 0;
                // Simple integer parsing
                int i = 0;
                bool negative = false;
                if (input[0] == '-') {
                    negative = true;
                    i = 1;
                }
                while (input[i] >= '0' && input[i] <= '9') {
                    value = value * 10 + (input[i] - '0');
                    i++;
                }
                if (negative) value = -value;
                mpop_set_operand_value(cpu, &instr->operand1, value);
            }
            break;
            
        case MPOP_HALT:
            cpu->running = false;
            return MPOP_SUCCESS;
            
        default:
            return MPOP_ERROR_INVALID_OPCODE;
    }
    
    cpu->pc++;
    return MPOP_SUCCESS;
}

int mpop_run(mpop_cpu_t* cpu) {
    if (!cpu) return MPOP_ERROR_PARSE_ERROR;
    
    cpu->running = true;
    cpu->pc = 0;
    
    int instruction_count = 0;
    const int MAX_INSTRUCTIONS = 10000; // Prevent infinite loops
    
    while (cpu->running && instruction_count < MAX_INSTRUCTIONS) {
        int result = mpop_step(cpu);
        if (result != MPOP_SUCCESS) {
            return result;
        }
        instruction_count++;
        
        if (cpu->debug_mode) {
            mpop_debug_print(cpu);
            printr("Press Enter to continue...\n");
            char temp[2];
            keyboard_input(temp);
        }
    }
    
    if (instruction_count >= MAX_INSTRUCTIONS) {
        printr("Warning: Maximum instruction limit reached\n");
    }
    
    return MPOP_SUCCESS;
}

void mpop_set_debug(mpop_cpu_t* cpu, bool debug) {
    if (cpu) {
        cpu->debug_mode = debug;
    }
}

void mpop_debug_print(mpop_cpu_t* cpu) {
    if (!cpu) return;
    
    printr("\n=== MPOP CPU State ===\n");
    printr("PC: %d, SP: %d\n", cpu->pc, cpu->sp);
    printr("Flags: Z=%d N=%d C=%d\n", cpu->zero_flag, cpu->negative_flag, cpu->carry_flag);
    
    printr("Registers:\n");
    for (int i = 0; i < MPOP_REGISTER_COUNT; i += 4) {
        printr("R%d:%d R%d:%d R%d:%d R%d:%d\n", 
              i, cpu->registers[i], i+1, cpu->registers[i+1],
              i+2, cpu->registers[i+2], i+3, cpu->registers[i+3]);
    }
    
    printr("Stack (top 8):\n");
    for (int i = cpu->sp - 1; i >= 0 && i >= cpu->sp - 8; i--) {
        printr("  [%d]: %d\n", i, cpu->stack[i]);
    }
    printr("=====================\n\n");
}

void mpop_command(int argc, char* argv[]) {
    char input[COMMAND_BUFFER_SIZE];
    mpop_cpu_t* cpu = mpop_init();
    
    if (!cpu) {
        printr("Error: Failed to initialize MPOP CPU\n");
        return;
    }
    
    printr("\nMPOP Virtual CPU\n");
    printr("Commands:\n");
    printr("  load <program>  - Load program (hello/fib/fact/test)\n");
    printr("  run             - Execute loaded program\n");
    printr("  step            - Execute single instruction\n");
    printr("  debug on/off    - Toggle debug mode\n");
    printr("  reset           - Reset CPU state\n");
    printr("  regs            - Show registers\n");
    printr("  help            - Show this help\n");
    printr("  mem             - Shows memory dump (first 64 bytes)\n");
    printr("  set             - Sets the register value\n");
    printr("  stack           - Loads stack\n");
    printr("  labels          - Shows labels\n");
    printr("  tutorial        - Shows tutorial\n");
    printr("  exit            - Exit MPOP\n\n");
    
    while (true) {
        printr("mpop> ");
        keyboard_input(input);
        
        if (strcmp(input, "exit") == 0) {
            break;
        } else if (strcmp(input, "help") == 0) {
            printr("MPOP Instruction Set:\n");
            printr("Data: MOV, LOAD, STORE\n");
            printr("Math: ADD, SUB, MUL, DIV, MOD, INC, DEC\n");
            printr("Logic: AND, OR, XOR, NOT, SHL, SHR\n");
            printr("Compare: CMP, TEST\n");
            printr("Jump: JMP, JZ, JNZ, JE, JNE, JL, JG\n");
            printr("Stack: PUSH, POP, CALL, RET\n");
            printr("I/O: printr, PRINTC, printrS, INPUT\n");
            printr("Control: HALT\n");
        } else if (strcmp(input, "reset") == 0) {
            mpop_reset(cpu);
            printr("CPU reset\n");
        } else if (strcmp(input, "regs") == 0) {
            mpop_debug_print(cpu);
        } else if (strcmp(input, "debug on") == 0) {
            mpop_set_debug(cpu, true);
            printr("Debug mode enabled\n");
        } else if (strcmp(input, "debug off") == 0) {
            mpop_set_debug(cpu, false);
            printr("Debug mode disabled\n");
        } else if (strcmp(input, "run") == 0) {
            if (cpu->program_size == 0) {
                printr("No program loaded\n");
                continue;
            }
            printr("Running program...\n");
            int result = mpop_run(cpu);
            if (result != MPOP_SUCCESS) {
                            } else {
                printr("Program completed successfully\n");
            }
        } else if (strcmp(input, "step") == 0) {
            if (cpu->program_size == 0) {
                printr("No program loaded\n");
                continue;
            }
            if (cpu->pc >= cpu->program_size) {
                printr("Program finished\n");
                continue;
            }
            int result = mpop_step(cpu);
            if (result != MPOP_SUCCESS) {
                printr("Execution error: %s\n", mpop_get_error_string(result));
            }
            mpop_debug_print(cpu);
        } else if (strcmp(input, "load hello") == 0) {
            int result = mpop_load_program(cpu, MPOP_HELLO_WORLD);
            if (result == MPOP_SUCCESS) {
                printr("Hello World program loaded (%d instructions)\n", cpu->program_size);
            } else {
                printr("Failed to load program: %s\n", mpop_get_error_string(result));
            }
        } else if (strcmp(input, "load fib") == 0) {
            int result = mpop_load_program(cpu, MPOP_FIBONACCI);
            if (result == MPOP_SUCCESS) {
                printr("Fibonacci program loaded (%d instructions)\n", cpu->program_size);
            } else {
                printr("Failed to load program: %s\n", mpop_get_error_string(result));
            }
        } else if (strcmp(input, "load fact") == 0) {
            int result = mpop_load_program(cpu, MPOP_FACTORIAL);
            if (result == MPOP_SUCCESS) {
                printr("Factorial program loaded (%d instructions)\n", cpu->program_size);
            } else {
                printr("Failed to load program: %s\n", mpop_get_error_string(result));
            }
        } 
        else if (strcmp(input, "load test") == 0) {
            int result = mpop_load_program(cpu, MPOP_TESTr);
            if (result == MPOP_SUCCESS) {
                printr("Test program loaded (%d instructions)\n", cpu->program_size);
            } else {
                printr("Failed to load program: %s\n", mpop_get_error_string(result));
            }
        } else if (strncmp(input, "load ", 5) == 0) {
            // Custom program loading
            char* program = input + 5;
            if (strlen(program) == 0) {
                printr("Usage: load <program_text>\n");
                printr("Example: load MOV R0, 42\nprintr R0\nHALT\n");
                continue;
            }
            
            // Replace \\n with actual newlines
            char processed_program[1024];
            int src = 0, dst = 0;
            while (program[src] && dst < 1023) {
                if (program[src] == '\\' && program[src + 1] == 'n') {
                    processed_program[dst++] = '\n';
                    src += 2;
                } else {
                    processed_program[dst++] = program[src++];
                }
            }
            processed_program[dst] = '\0';
            
            int result = mpop_load_program(cpu, processed_program);
            if (result == MPOP_SUCCESS) {
                printr("Custom program loaded (%d instructions)\n", cpu->program_size);
            } else {
                printr("Failed to load program: %s\n", mpop_get_error_string(result));
            }
        } else if (strcmp(input, "examples") == 0) {
            printr("Available example programs:\n");
            printr("  hello - Hello World\n");
            printr("  fib   - Fibonacci sequence\n");
            printr("  fact  - Factorial calculation\n");
            printr("\nCustom program syntax:\n");
            printr("  load MOV R0, 42\nprintr R0\nHALT\n");
        } else if (strcmp(input, "mem") == 0) {
            printr("Memory dump (first 64 bytes):\n");
            for (int i = 0; i < 64; i += 8) {
                printr("%03d: ", i);
                for (int j = 0; j < 8 && i + j < 64; j++) {
                    printr("%02X ", cpu->memory[i + j]);
                }
                printr("\n");
            }
        } else if (strncmp(input, "mem ", 4) == 0) {
            int addr = 0;
            char* addr_str = input + 4;
            while (*addr_str >= '0' && *addr_str <= '9') {
                addr = addr * 10 + (*addr_str - '0');
                addr_str++;
            }
            
            if (addr >= 0 && addr < MPOP_MEMORY_SIZE) {
                printr("Memory at address %d: %d (0x%02X)\n", 
                      addr, cpu->memory[addr], cpu->memory[addr]);
            } else {
                printr("Invalid memory address\n");
            }
        } else if (strncmp(input, "set ", 3) == 0) {
            // Set register value: set R0 42
            char* params = input + 4;
            if (*params == 'R' || *params == 'r') {
                params++;
                int reg = 0;
                while (*params >= '0' && *params <= '9') {
                    reg = reg * 10 + (*params - '0');
                    params++;
                }
                
                while (*params == ' ') params++;
                
                int value = 0;
                bool negative = false;
                if (*params == '-') {
                    negative = true;
                    params++;
                }
                
                while (*params >= '0' && *params <= '9') {
                    value = value * 10 + (*params - '0');
                    params++;
                }
                
                if (negative) value = -value;
                
                if (MPOP_IS_VALID_REGISTER(reg)) {
                    cpu->registers[reg] = value;
                    printr("R%d = %d\n", reg, value);
                } else {
                    printr("Invalid register number\n");
                }
            } else {
                printr("Usage: set R<num> <value>\n");
                printr("Example: set R0 42\n");
            }
        } else if (strcmp(input, "stack") == 0) {
            printr("Stack (SP = %d):\n", cpu->sp);
            if (cpu->sp == 0) {
                printr("  (empty)\n");
            } else {
                for (int i = cpu->sp - 1; i >= 0; i--) {
                    printr("  [%d]: %d\n", i, cpu->stack[i]);
                }
            }
        } else if (strcmp(input, "labels") == 0) {
            printr("Labels (%d total):\n", cpu->label_count);
            for (uint32_t i = 0; i < cpu->label_count; i++) {
                printr("  %s -> %d\n", cpu->labels[i].name, cpu->labels[i].address);
            }
        } else if (strcmp(input, "program") == 0) {
            printr("Loaded program (%d instructions):\n", cpu->program_size);
            for (uint32_t i = 0; i < cpu->program_size; i++) {
                char marker = (i == cpu->pc) ? '>' : ' ';
                printr("%c %03d: opcode=%d\n", marker, i, cpu->program[i].opcode);
            }
        } else if (strcmp(input, "tutorial") == 0) {
            printr("MPOP Tutorial:\n");
            printr("1. Load a program: load hello\n");
            printr("2. Enable debug: debug on\n");
            printr("3. Step through: step (repeat)\n");
            printr("4. Or run all: run\n");
            printr("5. Check state: regs\n");
            printr("\nSimple program example:\n");
            printr("load MOV R0, 10\nMOV R1, 5\nADD R0, R1\nprintr R0\nHALT\n");
        } else {
            printr("Unknown command. Type 'help' for available commands.\n");
        }
    }
    
    mpop_cleanup(cpu);
}

// Additional utility functions for enhanced functionality

const char* MPOP_BUBBLE_SORT = 
    "; Bubble sort example\n"
    "MOV R0, 5\n"        // Array size
    "MOV R1, 0\n"        // Outer loop counter
    "outer_loop:\n"
    "CMP R1, R0\n"
    "JE done\n"
    "MOV R2, 0\n"        // Inner loop counter
    "inner_loop:\n"
    "MOV R3, R0\n"
    "SUB R3, R1\n"
    "SUB R3, 1\n"
    "CMP R2, R3\n"
    "JE next_outer\n"
    "; Compare adjacent elements\n"
    "LOAD R4, R2\n"      // Load arr[j]
    "MOV R5, R2\n"
    "INC R5\n"
    "LOAD R6, R5\n"      // Load arr[j+1]
    "CMP R4, R6\n"
    "JL no_swap\n"
    "; Swap elements\n"
    "STORE R2, R6\n"     // arr[j] = arr[j+1]
    "STORE R5, R4\n"     // arr[j+1] = arr[j]
    "no_swap:\n"
    "INC R2\n"
    "JMP inner_loop\n"
    "next_outer:\n"
    "INC R1\n"
    "JMP outer_loop\n"
    "done:\n"
    "HALT\n";

// Function to validate program syntax
int mpop_validate_program(const char* assembly) {
    if (!assembly) return MPOP_ERROR_PARSE_ERROR;
    
    // Basic validation - check for balanced labels and basic syntax
    int line_count = 0;
    int label_count = 0;
    
    char line[256];
    int line_pos = 0;
    int asm_pos = 0;
    
    while (assembly[asm_pos]) {
        // Read line
        line_pos = 0;
        while (assembly[asm_pos] && assembly[asm_pos] != '\n' && line_pos < 255) {
            line[line_pos++] = assembly[asm_pos++];
        }
        line[line_pos] = '\0';
        
        if (assembly[asm_pos] == '\n') asm_pos++;
        
        // Skip empty lines and comments
        if (line_pos == 0 || line[0] == ';') continue;
        
        line_count++;
        
        // Check for label
        if (line[line_pos - 1] == ':') {
            label_count++;
        }
        
        // Basic opcode validation could be added here
    }
    
    if (line_count > MPOP_MAX_CODE_SIZE) {
        return MPOP_ERROR_PROGRAM_TOO_LARGE;
    }
    
    return MPOP_SUCCESS;
}

// Disassembler function
void mpop_disassemble(mpop_cpu_t* cpu) {
    if (!cpu || cpu->program_size == 0) {
        printr("No program loaded\n");
        return;
    }
    
    const char* opcode_names[] = {
        "NOP", "MOV", "LOAD", "STORE",
        "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???",
        "ADD", "SUB", "MUL", "DIV", "MOD", "INC", "DEC",
        "???", "???", "???", "???", "???", "???", "???", "???", "???",
        "AND", "OR", "XOR", "NOT", "SHL", "SHR",
        "???", "???", "???", "???", "???", "???", "???", "???", "???", "???",
        "CMP", "TEST",
        "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???",
        "JMP", "JZ", "JNZ", "JE", "JNE", "JL", "JG", "CALL", "RET",
        "???", "???", "???", "???", "???", "???", "???",
        "PUSH", "POP",
        "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???",
        "PRINT", "PRINTC", "PRINTS", "INPUT"
    };
    
    printr("Disassembly:\n");
    for (uint32_t i = 0; i < cpu->program_size; i++) {
        mpop_instruction_t* instr = &cpu->program[i];
        char marker = (i == cpu->pc) ? '>' : ' ';
        
        if (instr->opcode < sizeof(opcode_names)/sizeof(opcode_names[0])) {
            printr("%c %03d: %s\n", marker, i, opcode_names[instr->opcode]);
        } else if (instr->opcode == MPOP_HALT) {
            printr("%c %03d: HALT\n", marker, i);
        } else {
            printr("%c %03d: UNKNOWN(%d)\n", marker, i, instr->opcode);
        }
    }
}


