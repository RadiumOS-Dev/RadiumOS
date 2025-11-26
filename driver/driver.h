#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include <stdbool.h>


#define MAX_DRIVERS 10
#define DRIVER_NAME_LENGTH 32

typedef struct {
    char name[DRIVER_NAME_LENGTH];
    void (*function)(void); // Function pointer for the driver's operation
    bool enabled;           // Driver status
    char developer[DRIVER_NAME_LENGTH];
    uint32_t time_of_register; // Time of registration
} Driver;

void driver_set(const char *name, void (*function)(void), const char *developer);
void driver_enable(const char *name);
void driver_disable(const char *name);
void driver_check(const char *name);
void driver_load(const char *name, void (*function)(void), const char *developer);
void driver_unload(const char *name);

#endif // DRIVER_H
