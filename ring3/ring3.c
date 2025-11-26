#include "ring3.h"
#include "../io/io.h" // For I/O operations
#include "../memory/memory.h" // For memory management functions

// Function to switch to Ring 3
void jump_to_ring3(void (*user_program)()) {
    // Set up the user mode stack
    uint32_t user_stack = 0x9FFFFFFC; // Example stack pointer in user space
    setup_user_stack(user_stack);
    setup_user_segments(); // Set up user mode segment selectors

    // Load the user program's segment selectors and jump to the user program
    asm volatile (
        "movl %0, %%eax\n"        // Load the user program address into EAX
        "jmp *%%eax\n"            // Jump to the user program
        : // No output
        : "r"(user_program)       // Input: user program address
        : "%eax"                  // Clobbered register
    );
}

// Function to set up the user mode stack
void setup_user_stack(uint32_t stack_pointer) {
    // Set the stack pointer for user mode
    asm volatile (
        "movl %0, %%esp\n"        // Move the stack pointer to ESP
        : // No output
        : "r"(stack_pointer)      // Input: user stack pointer
        : "%esp"                  // Clobbered register
    );
}

// Function to set up the user mode segment selectors
void setup_user_segments() {
    // Set up the user mode segment selectors
    asm volatile (
        "movw $0x23, %%ax\n"      // Load the user data segment selector (0x23)
        "movw %%ax, %%ds\n"       // Set DS to user data segment
        "movw %%ax, %%es\n"       // Set ES to user data segment
        "movw %%ax, %%fs\n"       // Set FS to user data segment
        "movw %%ax, %%gs\n"       // Set GS to user data segment
        "movw $0x1B, %%ax\n"      // Load the user code segment selector (0x1B)
        "movw %%ax, %%cs\n"       // Set CS to user code segment
        : // No output
        : // No input
        : "%ax"                   // Clobbered register
    );
}
