#ifndef JUMP_TO_RING3_H
#define JUMP_TO_RING3_H

#include <stdint.h>

// Function to switch to Ring 3
void jump_to_ring3(void (*user_program)());

// Function to set up the user mode stack
void setup_user_stack(uint32_t stack_pointer);

// Function to set up the user mode segment selectors
void setup_user_segments();

#endif // JUMP_TO_RING3_H
