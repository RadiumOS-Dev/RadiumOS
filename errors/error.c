#include "error.h"
#include "../terminal/terminal.h"
#include <stdbool.h> // For true/false definitions
#include "../utility/utility.h" // For system utilities functions
#include "../sound/sound.h"
extern uint8_t terminal_color; // Assuming terminal_color is of type uint8_t
void meltdown_screen(char* message, char* file, int line, int error_code, int cr2, int int_no){
    vga_window_t main_win = vga_create_centered_window(
        100, 100, 
        VGA_COLOR_WHITE, 
        VGA_COLOR_RED  // Changed to RED for error display
    );
    
    // Set title for error screen
    vga_win_set_title(&main_win, "Meltdown - RadiumKernel");
    
    // Buffer for formatted strings
    char buffer[256];
    
    // Add error content
    int row = 2;
    vga_win_puts_centered(&main_win, row++, "__~~==[ Meltdown Occurred at RadiumKernel! ]===");
    row++;
    
    vga_win_puts(&main_win, row++, 5, "Error Message:");
    vga_win_puts(&main_win, row++, 7, message);
    row++;
    
    // Format error code
    sprintf(buffer, "Error Code: 0x%x", error_code);
    vga_win_puts(&main_win, row++, 5, buffer);
    
    // Format CR2
    sprintf(buffer, "CR2: 0x%x (%d)", cr2, cr2);
    vga_win_puts(&main_win, row++, 5, buffer);
    
    // Format interrupt number
    sprintf(buffer, "Interrupt No.: 0x%x", int_no);
    vga_win_puts(&main_win, row++, 5, buffer);
    row++;
    
    // Handler information
    vga_win_puts(&main_win, row++, 5, "=[ Handler Information ]=");
    sprintf(buffer, "File name: %s", file);
    vga_win_puts(&main_win, row++, 5, buffer);
    sprintf(buffer, "Line number: %d", line);
    vga_win_puts(&main_win, row++, 5, buffer);
    
    // Refresh to display
    vga_win_refresh(&main_win);
    
    // Wait with error sound
    while (true) {
        speaker_play_error_sound();
    }
    
    // This will never execute due to infinite loop
    vga_destroy_window(&main_win);
}
void handle_error(const char* message, const char* errorType) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    print(message);
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    
    if (strcmp(errorType, "kernel") == 0) {
        print("\nNop -+ %x901_: Kernel Panic - System Halted\n");
        while (true) {
            // Infinite loop for kernel errors
        }
    }
    else if (strcmp(errorType, "Hardware") == 0) {
        print("\nNop -+ %x023_: Hardware Fault Detected\n");
        while (true) {
            // Infinite loop for hardware errors
        }
    }
    else if (strcmp(errorType, "memory") == 0) {
        print("\nNop -+ %x256_: Memory Error - System Unstable\n");
        // Attempt recovery or halt
        while (true) {
            //
        }
    }
    else if (strcmp(errorType, "task") == 0) {
        print("\nNop -+ %x128_: Task Error - Process Terminated\n");
        // Could potentially recover from task errors
        return;
    }
    else if (strcmp(errorType, "system") == 0) {
        print("\nNop -+ %x512_: System Error - Operation Failed\n");
        // System errors might be recoverable
        return;
    }
    else {
        print("\nInvalid Command !\n");
    }
}

void handle_specific_error(const char* errorCode, const char* description) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    print("ERROR: ");
    print(errorCode);
    print(" - ");
    print(description);
    print("\n");
    asm volatile("cli");
    
    while (true) {
        asm volatile("hlt"); // Halt the processor
    }
}

void kernel_panic(const char* reason, const char* errorCode) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    print("\n*** KERNEL PANIC ***\n");
    print("Reason: ");
    print(reason);
    print("\nError Code: ");
    print(errorCode);
    print("\nSystem Halted - Please restart\n");
    
    // Disable interrupts
    asm volatile("cli");
    
    while (true) {
        asm volatile("hlt"); // Halt the processor
    }
}

void hardware_fault(const char* component, const char* errorCode) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_RED));
    print("\n*** HARDWARE FAULT ***\n");
    print("Component: ");
    print(component);
    print("\nError Code: ");
    print(errorCode);
    print("\nHoS -+ Hardware operation suspended\n");
    
    asm volatile("cli");
    
    while (true) {
        asm volatile("hlt"); // Halt the processor
    }
}



void memory_error(const char* operation, const char* errorCode) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    print("\n*** MEMORY ERROR ***\n");
    print("Operation: ");
    print(operation);
    print("\nError Code: ");
    print(errorCode);
    print("\nMoF -+ Memory operation failed\n");
}

void task_error(const char* taskName, const char* errorCode) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    print("\n*** TASK ERROR ***\n");
    print("Task: ");
    print(taskName);
    print("\nError Code: ");
    print(errorCode);
    print("\nToF -+ Task operation failed\n");
    asm volatile("cli");
    
    while (true) {
        asm volatile("hlt"); // Halt the processor
    }
}

void system_error(const char* operation, const char* errorCode) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
    print("\n*** SYSTEM ERROR ***\n");
    print("Operation: ");
    print(operation);
    print("\nError Code: ");
    print(errorCode);
    print("\nSoF -+ System operation failed\n");
    asm volatile("cli");
    
    while (true) {
        asm volatile("hlt"); // Halt the processor
    }
}

// Error recovery function
bool attempt_recovery(const char* errorType) {
    if (strcmp(errorType, "task") == 0) {
        print("Attempting task recovery...\n");
        // Add task recovery logic here
        return true;
    }
    else if (strcmp(errorType, "system") == 0) {
        print("Attempting system recovery...\n");
        // Add system recovery logic here
        return true;
    }
    return false; // Cannot recover from this error type
}

// Log error for debugging
void log_error(const char* errorCode, const char* message) {
    // This could write to a log buffer or serial port for debugging
    print("[LOG] ");
    print(errorCode);
    print(": ");
    print(message);
    print("\n");
    asm volatile("cli");
    
    while (true) {
        asm volatile("hlt"); // Halt the processor
    }
}
