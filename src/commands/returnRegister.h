#ifndef REGISTER_RETURNER_H
#define REGISTER_RETURNER_H
#include <stdint.h>
//#define COMMAND_BUFFER_SIZE 256
// Function to convert an integer to a hexadecimal string
void intToHexString(uint8_t value, char* buffer);
// Function to handle user commands related to register operations
void returnRegister_command(int argc, char* argv[]);
#endif // REGISTER_RETURNER_H