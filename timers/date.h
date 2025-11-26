// date.h - Fixed RTC header
#ifndef DATE_H
#define DATE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} datetime_t;

// Initialize RTC
int rtc_init(void);

// Get current time (in local timezone - EST)
void rtc_get_time(datetime_t *dt);

// Apply timezone offset to datetime
void apply_timezone_offset(datetime_t *dt, int offset_hours);

// Get days in a specific month
uint8_t days_in_month(uint8_t month, uint16_t year);

// Get Unix timestamp (UTC)
uint32_t get_unix_timestamp(void);

// Format time in 12-hour format with AM/PM
void format_time_12h(datetime_t *dt, char *buffer);

// Format time in 24-hour format
void format_time_24h(datetime_t *dt, char *buffer);

// Format date as "Mon DD, YYYY"
void format_date(datetime_t *dt, char *buffer);

// Command handlers
void date_command(int argc, char* argv[]);
void time_command(int argc, char* argv[]);
void timestamp_command(int argc, char* argv[]);

// Helper functions
void get_time_string(char* buffer, bool use_24h);
void get_date_string(char* buffer);

#endif // DATE_H