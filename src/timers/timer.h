#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

// Timer constants
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

// Global tick counter
extern volatile uint32_t ticks;

// Core timer functions
void timer_interrupt_handler(void);
void init_timer(void);
void delay(int milliseconds);
void precise_delay(uint32_t milliseconds);

// Delay functions
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);
void delay_us_precise(uint32_t us);

// Time retrieval functions
uint32_t get_time_ms(void);
uint64_t get_time_us(void);
uint64_t get_time_ns(void);

// Uptime functions
void uptime_task(uint32_t id);
int get_uptime(void);
uint32_t get_uptime_ms(void);
uint32_t get_ticks(void);
void get_uptime_precise(uint32_t* days, uint32_t* hours, uint32_t* minutes, uint32_t* seconds, uint32_t* milliseconds);

// Sleep functions (aliases)
void sleep_ms(uint32_t ms);
void sleep_us(uint32_t us);
void sleep_seconds(uint32_t seconds);

// Debug functions
void debug_timer_frequency(void);


#endif // TIMER_H
