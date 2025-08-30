#include "timer.h"
#include "../scheduler/task.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../io/io.h"

volatile uint32_t ticks = 0; // Global tick counter

void timer_interrupt_handler() {
    ticks++;
    
    // Handle scheduling every timer tick
    if (ticks % 10 == 0) {  // Schedule every 10ms (adjust as needed)
        schedule();
    }
    
    outb(0x20, 0x20); // Send EOI to PIC
}

void init_timer() {
    setup_pit(1000); // Initialize PIT at 1000 Hz (1ms per tick)
}

void delay(int milliseconds) {
    uint32_t start_ticks = ticks;
    uint32_t target_ticks = start_ticks + milliseconds;
    
    // Wait until enough ticks have passed
    while (ticks < target_ticks) {
        // Yield to other tasks while waiting
        asm volatile("hlt");
    }
}

// More accurate delay using timer ticks
void precise_delay(uint32_t milliseconds) {
    uint32_t start = ticks;
    while ((ticks - start) < milliseconds) {
        asm volatile("hlt");
    }
}

// Delay in milliseconds (alias for delay function)
void delay_ms(uint32_t ms) {
    precise_delay(ms);
}

// Delay in microseconds (approximate - limited by timer resolution)
void delay_us(uint32_t us) {
    if (us < 1000) {
        // For microsecond delays less than 1ms, use busy waiting
        // This is approximate and depends on CPU speed
        volatile uint32_t count = us * 1000; // Rough calibration needed
        while (count--) {
            asm volatile("nop");
        }
    } else {
        // For delays >= 1ms, use the millisecond delay
        delay_ms(us / 1000);
    }
}

// Get current time in milliseconds since boot
uint32_t get_time_ms(void) {
    return ticks; // Direct tick count = milliseconds at 1000 Hz
}

// The uptime_task displays uptime periodically
void uptime_task(uint32_t id) {
    uint8_t display_counter = 0;
    uint32_t last_display_time = 0;
    
    while (true) {
        uint32_t current_time = ticks;
        
        // Update display every second (1000 ticks at 1000 Hz)
        if (current_time - last_display_time >= 1000) {
            // Visual indicator that uptime task is running
            *(VGA_MEMORY + 160 + id) = 0x0A00 | (display_counter++ % 10) + '0';
            last_display_time = current_time;
        }
        
        // Sleep for a short time to avoid consuming too much CPU
        precise_delay(100); // 100ms delay
    }
}

// Function to get the current uptime in seconds
int get_uptime() {
    return ticks / 1000; // Convert ticks to seconds (1000 ticks = 1 second)
}

// Function to get uptime in milliseconds
uint32_t get_uptime_ms() {
    return ticks; // Direct tick count = milliseconds at 1000 Hz
}

// Function to get raw tick count
uint32_t get_ticks() {
    return ticks;
}

// Function to get uptime with more precision
void get_uptime_precise(uint32_t* days, uint32_t* hours, uint32_t* minutes, uint32_t* seconds, uint32_t* milliseconds) {
    uint32_t total_ms = ticks;
    uint32_t total_seconds = total_ms / 1000;
    
    *milliseconds = total_ms % 1000;
    *seconds = total_seconds % 60;
    *minutes = (total_seconds / 60) % 60;
    *hours = (total_seconds / 3600) % 24;
    *days = total_seconds / 86400;
}

// Debug function to check timer frequency
void debug_timer_frequency() {
    static uint32_t last_check = 0;
    static uint32_t check_count = 0;
    
    uint32_t current = ticks;
    if (last_check != 0) {
        uint32_t delta = current - last_check;
        print("Timer delta: ");
        print_decimal(delta);
        print(" ticks\n");
    }
    
    last_check = current;
    check_count++;
}

// High-resolution delay using PIT directly (for very precise timing)
void delay_us_precise(uint32_t us) {
    // For very precise microsecond delays, we can use the PIT counter directly
    // This is more accurate than the busy-wait method above
    
    if (us == 0) return;
    
    // Calculate PIT ticks needed (PIT runs at ~1.193182 MHz)
    uint32_t pit_ticks = (us * 1193182) / 1000000;
    
    if (pit_ticks < 65536) {
        // Use PIT channel 2 for precise timing
        outb(0x43, 0xB2); // Configure PIT channel 2
        outb(0x42, pit_ticks & 0xFF);
        outb(0x42, (pit_ticks >> 8) & 0xFF);
        
        // Enable speaker gate to start counting
        uint8_t speaker = inb(0x61);
        outb(0x61, speaker | 0x03);
        
        // Wait for counting to complete
        while (!(inb(0x61) & 0x20));
        
        // Disable speaker gate
        outb(0x61, speaker);
    } else {
        // For longer delays, fall back to millisecond delay
        delay_ms(us / 1000);
        if (us % 1000) {
            delay_us_precise(us % 1000);
        }
    }
}

// Get system time in different formats
uint64_t get_time_us(void) {
    return (uint64_t)ticks * 1000; // Convert ms to us
}

uint64_t get_time_ns(void) {
    return (uint64_t)ticks * 1000000; // Convert ms to ns
}

// Sleep functions (aliases for delay functions)
void sleep_ms(uint32_t ms) {
    delay_ms(ms);
}

void sleep_us(uint32_t us) {
    delay_us(us);
}

void sleep_seconds(uint32_t seconds) {
    delay_ms(seconds * 1000);
}
