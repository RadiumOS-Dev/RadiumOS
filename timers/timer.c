#include "timer.h"
#include "../scheduler/task.h"
#include "../terminal/terminal.h"
#include "../utility/utility.h"
#include "../io/io.h"

volatile uint32_t ticks = 0; // Global tick counter (1 tick = 1ms)

void timer_interrupt_handler() {
    ticks++;
    
    // Handle scheduling every 10ms
    if (ticks % 10 == 0) {
        schedule();
    }
    
    outb(0x20, 0x20); // Send EOI to PIC
}

void init_timer() {
    setup_pit(1000); // Initialize PIT at 1000 Hz (1ms per tick)
}

// Basic delay using timer ticks
void delay(int milliseconds) {
    if (milliseconds <= 0) return;
    
    uint32_t start_ticks = ticks;
    uint32_t target_ticks = start_ticks + (uint32_t)milliseconds;
    
    // Safety: Check if timer is running
    uint32_t check = ticks;
    for (volatile int i = 0; i < 100000; i++);
    if (ticks == check) {
        // Timer not incrementing, use busy wait fallback
        for (volatile uint32_t i = 0; i < (uint32_t)milliseconds * 100000UL; i++);
        return;
    }
    
    // Handle wraparound case
    if (target_ticks < start_ticks) {
        // Wait for wraparound
        while (ticks >= start_ticks) {
            asm volatile("hlt");
        }
    }
    
    // Wait until target reached
    while (ticks < target_ticks) {
        asm volatile("hlt");
    }
}

// More accurate delay using subtraction (handles wraparound automatically)
void precise_delay(uint32_t milliseconds) {
    if (milliseconds == 0) return;
    
    uint32_t start = ticks;
    uint32_t elapsed = 0;
    
    // Safety check: ensure ticks is actually incrementing
    uint32_t safety_check = ticks;
    for (volatile int i = 0; i < 100000; i++);
    if (ticks == safety_check) {
        // Timer not running! Fall back to busy wait (not ideal but better than nothing)
        for (volatile uint32_t i = 0; i < milliseconds * 100000UL; i++);
        return;
    }
    
    while (elapsed < milliseconds) {
        asm volatile("hlt");
        elapsed = ticks - start;
    }
}

// Delay in milliseconds
void delay_ms(uint32_t ms) {
    precise_delay(ms);
}

// High-resolution delay using PIT channel 2
void delay_us_precise(uint32_t us) {
    if (us == 0) return;
    
    // For delays >= 1ms, use regular delay for better accuracy
    if (us >= 1000) {
        uint32_t ms = us / 1000;
        uint32_t us_remainder = us % 1000;
        if (ms > 0) delay_ms(ms);
        if (us_remainder > 0) delay_us_precise(us_remainder);
        return;
    }
    
    // Calculate PIT ticks needed (PIT base frequency: 1193182 Hz)
    uint32_t pit_ticks = (us * 1193182UL + 500000UL) / 1000000UL;
    if (pit_ticks == 0) pit_ticks = 1;
    if (pit_ticks > 65535) pit_ticks = 65535;
    
    // Disable interrupts during critical section
    asm volatile("cli");
    
    // Save original port 0x61 state
    uint8_t speaker_state = inb(0x61);
    
    // Configure PIT channel 2 for one-shot mode (mode 0)
    // Command byte: 10110000b = Channel 2, Low/High byte access, Mode 0, Binary
    outb(0x43, 0xB0);
    
    // Write counter value (low byte first, then high byte)
    outb(0x42, (uint8_t)(pit_ticks & 0xFF));
    outb(0x42, (uint8_t)((pit_ticks >> 8) & 0xFF));
    
    // Start counter by enabling gate
    outb(0x61, (speaker_state & 0xFC) | 0x01);
    
    // Re-enable interrupts
    asm volatile("sti");
    
    // Poll for completion (bit 5 goes high when count reaches zero)
    // Add timeout to prevent infinite loop
    uint32_t timeout = 1000000;
    while (!(inb(0x61) & 0x20) && timeout > 0) {
        timeout--;
        asm volatile("pause"); // CPU hint for spin-wait loop
    }
    
    // Restore original speaker state
    outb(0x61, speaker_state);
}

// Delay in microseconds
void delay_us(uint32_t us) {
    if (us == 0) return;
    
    if (us >= 1000) {
        // For larger delays, use millisecond delay for better accuracy
        delay_ms(us / 1000);
        uint32_t us_remainder = us % 1000;
        if (us_remainder > 0) {
            delay_us_precise(us_remainder);
        }
    } else {
        // For sub-millisecond delays, use precise PIT method
        delay_us_precise(us);
    }
}

// Get current time in milliseconds since boot
uint32_t get_time_ms(void) {
    return ticks;
}

// Get system time in different formats
uint64_t get_time_us(void) {
    return (uint64_t)ticks * 1000ULL;
}

uint64_t get_time_ns(void) {
    return (uint64_t)ticks * 1000000ULL;
}

// Function to get the current uptime in seconds
int get_uptime() {
    return ticks / 1000;
}

// Function to get uptime in milliseconds
uint32_t get_uptime_ms() {
    return ticks;
}

// Function to get raw tick count
uint32_t get_ticks() {
    return ticks;
}

// Function to get uptime with more precision
void get_uptime_precise(uint32_t* days, uint32_t* hours, uint32_t* minutes, 
                        uint32_t* seconds, uint32_t* milliseconds) {
    if (!days || !hours || !minutes || !seconds || !milliseconds) return;
    
    uint32_t total_ms = ticks;
    uint32_t total_seconds = total_ms / 1000;
    
    *milliseconds = total_ms % 1000;
    *seconds = total_seconds % 60;
    *minutes = (total_seconds / 60) % 60;
    *hours = (total_seconds / 3600) % 24;
    *days = total_seconds / 86400;
}

// Sleep functions (aliases)
void sleep_ms(uint32_t ms) {
    delay_ms(ms);
}

void sleep_us(uint32_t us) {
    delay_us(us);
}

void sleep_seconds(uint32_t seconds) {
    if (seconds == 0) return;
    
    // Sleep one second at a time to avoid any overflow issues
    for (uint32_t i = 0; i < seconds; i++) {
        precise_delay(1000); // Sleep 1 second (1000ms)
    }
}

// The uptime_task displays uptime periodically
void uptime_task(uint32_t id) {
    uint32_t last_display_time = 0;
    
    while (true) {
        uint32_t current_time = get_time_ms();
        
        // Update display every second (1000 ms)
        if (current_time - last_display_time >= 1000) {
            uint32_t days, hours, mins, secs, ms;
            get_uptime_precise(&days, &hours, &mins, &secs, &ms);
            
            print("Uptime: ");
            if (days > 0) {
                print_decimal(days);
                print("d ");
            }
            print_decimal(hours);
            print("h ");
            print_decimal(mins);
            print("m ");
            print_decimal(secs);
            print(".");
            
            // Print milliseconds with leading zeros
            if (ms < 100) print("0");
            if (ms < 10) print("0");
            print_decimal(ms);
            print("s\n");
            
            last_display_time = current_time;
        }
        
        // Sleep for a short time to avoid consuming too much CPU
        sleep_ms(100);
    }
}

// Debug function to check timer frequency
void debug_timer_frequency() {
    static uint32_t last_check = 0;
    static uint32_t check_count = 0;
    
    uint32_t current = ticks;
    if (last_check != 0) {
        uint32_t delta = current - last_check;
        if (delta > 0) {
            print("Timer delta: ");
            print_decimal(delta);
            print(" ms (check #");
            print_decimal(check_count);
            print(")\n");
        }
    }
    
    last_check = current;
    check_count++;
}