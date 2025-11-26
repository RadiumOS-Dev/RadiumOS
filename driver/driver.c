#include "driver.h"
#include "../terminal/terminal.h" // For printing
#include "../memory/memory.h"   // For memory management
#include "../timers/timer.h"    // Include the timer header

static Driver drivers[MAX_DRIVERS];
static int driver_count = 0;

void driver_set(const char *name, void (*function)(void), const char *developer) {
    if (driver_count >= MAX_DRIVERS) {
        terminal_setcolor(VGA_COLOR_RED);
        print("Driver registration failed: Maximum driver limit reached.\n");
        terminal_setcolor(VGA_COLOR_WHITE);
        return;
    }

    // Initialize the driver
    strncpy(drivers[driver_count].name, name, DRIVER_NAME_LENGTH);
    drivers[driver_count].function = function;
    drivers[driver_count].enabled = 0; // Initially OFF
    strncpy(drivers[driver_count].developer, developer, DRIVER_NAME_LENGTH);
    drivers[driver_count].time_of_register = get_ticks(); // Use timer ticks for registration time

    driver_count++;
}

void driver_load(const char *name, void (*function)(void), const char *developer) {
    driver_set(name, function, developer);
    driver_enable(name); // Automatically enable the driver after loading
}

void driver_enable(const char *name) {
    for (int i = 0; i < driver_count; i++) {
        if (strcmp(drivers[i].name, name) == 0) {
            if (drivers[i].enabled) {
                terminal_setcolor(VGA_COLOR_BLUE);
                print("Driver ");
                print(drivers[i].name);
                print(" is already enabled.\n");
                terminal_setcolor(VGA_COLOR_WHITE);
                return;
            }
            drivers[i].enabled = 1; // Set to ACTIVE
            
            // Output driver enable information
            terminal_setcolor(VGA_COLOR_CYAN);
            print("[");
            print_decimal(drivers[i].time_of_register);
            print("-Tick] : [");
            
            terminal_setcolor(VGA_COLOR_BLUE); // Set color for driver name
            print(drivers[i].name);
            
            terminal_setcolor(VGA_COLOR_WHITE); // Set color for status
            print("] : [ ");
            terminal_setcolor(VGA_COLOR_GREEN);
            print("ACTIVE");
            terminal_setcolor(VGA_COLOR_WHITE);
            print(" ] : [");
            print(drivers[i].developer);
            print("]\n");
            return;
        }
    }
    terminal_setcolor(VGA_COLOR_RED);
    print("KERNEL SECURITY FAULT: NO DRIVER SET FOR: ");
    print(name);
    print("\n");
    terminal_setcolor(VGA_COLOR_WHITE);
}

void driver_disable(const char *name) {
    for (int i = 0; i < driver_count; i++) {
        if (strcmp(drivers[i].name, name) == 0) {
            if (!drivers[i].enabled) {
                terminal_setcolor(VGA_COLOR_BLUE);
                print("Driver ");
                print(drivers[i].name);
                print(" is already disabled.\n");
                terminal_setcolor(VGA_COLOR_WHITE);
                return;
            }
            drivers[i].enabled = 0; // Set to INACTIVE
            terminal_setcolor(VGA_COLOR_CYAN);
            print("Driver ");
            print(drivers[i].name);
            print(" has been disabled.\n");
            terminal_setcolor(VGA_COLOR_WHITE);
            return;
        }
    }
    terminal_setcolor(VGA_COLOR_RED);
    print("KERNEL SECURITY FAULT: NO DRIVER SET FOR: ");
    print(name);
    print("\n");
    terminal_setcolor(VGA_COLOR_WHITE);
}

void driver_unload(const char *name) {
    driver_disable(name); // Disable the driver before unloading
    for (int i = 0; i < driver_count; i++) {
        if (strcmp(drivers[i].name, name) == 0) {
            // Shift remaining drivers down
            for (int j = i; j < driver_count - 1; j++) {
                drivers[j] = drivers[j + 1];
            }
            driver_count--; // Decrease the driver count
            terminal_setcolor(VGA_COLOR_CYAN);
            print("Driver ");
            print(name);
            print(" has been unloaded.\n");
            terminal_setcolor(VGA_COLOR_WHITE);
            return;
        }
    }
    terminal_setcolor(VGA_COLOR_RED);
    print("KERNEL SECURITY FAULT: NO DRIVER SET FOR: ");
    print(name);
    print("\n");
    terminal_setcolor(VGA_COLOR_WHITE);
}

void driver_check(const char *name) {
    for (int i = 0; i < driver_count; i++) {
        if (strcmp(drivers[i].name, name) == 0) {
            if (!drivers[i].enabled) {
                terminal_setcolor(VGA_COLOR_RED);
                print("Driver ");
                print(drivers[i].name);
                print(" failed to initialize. Please restart or wait for a new update.\n");
                terminal_setcolor(VGA_COLOR_WHITE);
                return;
            }
            // Call the driver's function if enabled
            drivers[i].function();
            return;
        }
    }
    terminal_setcolor(VGA_COLOR_RED);
    print("KERNEL SECURITY FAULT: NO DRIVER SET FOR: ");
    print(name);
    print("\n");
    terminal_setcolor(VGA_COLOR_WHITE);
}
