#ifndef SETTINGS_H
#define SETTINGS_H
#include <stddef.h> // For size_t
#include <stdbool.h>
// Declare the variable (extern means it's defined elsewhere)
extern bool is_debug;
extern bool is_verbose;
// Function prototypes
void settings_command(int argc, char* argv[]);
#endif // SETTINGS_H
