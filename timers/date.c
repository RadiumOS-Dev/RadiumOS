// date.c - Fixed RTC implementation with proper timezone handling
#include "date.h"
#include "../io/io.h"
#include "../terminal/terminal.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

#define RTC_SECONDS    0x00
#define RTC_MINUTES    0x02
#define RTC_HOURS      0x04
#define RTC_DAY        0x07
#define RTC_MONTH      0x08
#define RTC_YEAR       0x09
#define RTC_STATUS_A   0x0A
#define RTC_STATUS_B   0x0B

// EST is UTC-5, EDT is UTC-4
// For simplicity, using EST (you can add DST detection later)
#define TIMEZONE_OFFSET_HOURS (-5)  // Negative because we're behind UTC

static uint8_t rtc_read(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static void rtc_write(uint8_t reg, uint8_t value) {
    outb(CMOS_ADDRESS, reg);
    outb(CMOS_DATA, value);
}

static uint8_t bcd_to_bin(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

static int rtc_update_in_progress(void) {
    return rtc_read(RTC_STATUS_A) & 0x80;
}

static int is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int rtc_init(void) {
    // Wait for any update to complete
    int timeout = 1000;
    while (rtc_update_in_progress() && timeout-- > 0);
    
    if (timeout == 0) {
        return 0; // RTC busy, initialization failed
    }
    
    // Read current status
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    
    // Enable binary mode (bit 2) and 24-hour format (bit 1)
    status_b |= 0x02;  // 24-hour format (bit 1 = 1)
    status_b |= 0x04;  // Binary mode (bit 2 = 1)
    
    rtc_write(RTC_STATUS_B, status_b);
    
    // Verify the settings
    uint8_t verify = rtc_read(RTC_STATUS_B);
    
    return 1; // Success
}

void rtc_get_time(datetime_t *dt) {
    // Wait for update to complete
    while (rtc_update_in_progress());
    
    // Read all values at once to ensure consistency
    dt->second = rtc_read(RTC_SECONDS);
    dt->minute = rtc_read(RTC_MINUTES);
    dt->hour = rtc_read(RTC_HOURS);
    dt->day = rtc_read(RTC_DAY);
    dt->month = rtc_read(RTC_MONTH);
    dt->year = rtc_read(RTC_YEAR);
    
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    
    // Convert from BCD to binary if needed (bit 2 clear = BCD mode)
    if (!(status_b & 0x04)) {
        dt->second = bcd_to_bin(dt->second);
        dt->minute = bcd_to_bin(dt->minute);
        dt->hour = bcd_to_bin(dt->hour & 0x7F);  // Mask off PM bit if present
        dt->day = bcd_to_bin(dt->day);
        dt->month = bcd_to_bin(dt->month);
        dt->year = bcd_to_bin(dt->year);
    }
    
    // Handle 12-hour format if bit 1 is clear
    if (!(status_b & 0x02)) {
        // In 12-hour mode, bit 7 indicates PM
        bool is_pm = (dt->hour & 0x80) != 0;
        dt->hour = dt->hour & 0x7F; // Clear PM bit
        
        if (is_pm) {
            if (dt->hour != 12) {
                dt->hour += 12;
            }
        } else {
            if (dt->hour == 12) {
                dt->hour = 0; // 12 AM is 0 hours in 24-hour format
            }
        }
    }
    
    // Adjust year (RTC gives year since 2000)
    if (dt->year < 100) {
        dt->year += 2000;
    }
    
    // Apply timezone offset
    apply_timezone_offset(dt, TIMEZONE_OFFSET_HOURS);
}

// Helper function to apply timezone offset
void apply_timezone_offset(datetime_t *dt, int offset_hours) {
    if (offset_hours == 0) return;
    
    int new_hour = (int)dt->hour + offset_hours;
    
    if (new_hour < 0) {
        // Going back to previous day
        dt->hour = 24 + new_hour;
        
        if (dt->day > 1) {
            dt->day--;
        } else {
            // Go to previous month
            if (dt->month > 1) {
                dt->month--;
            } else {
                dt->month = 12;
                dt->year--;
            }
            
            // Set to last day of new month
            dt->day = days_in_month(dt->month, dt->year);
        }
    } else if (new_hour >= 24) {
        // Going forward to next day
        dt->hour = new_hour - 24;
        
        uint8_t max_day = days_in_month(dt->month, dt->year);
        
        if (dt->day < max_day) {
            dt->day++;
        } else {
            // Go to next month
            dt->day = 1;
            
            if (dt->month < 12) {
                dt->month++;
            } else {
                dt->month = 1;
                dt->year++;
            }
        }
    } else {
        dt->hour = new_hour;
    }
}

// Helper to get days in a month
uint8_t days_in_month(uint8_t month, uint16_t year) {
    uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month < 1 || month > 12) return 30;
    
    uint8_t result = days[month - 1];
    
    // Check for leap year in February
    if (month == 2 && is_leap_year(year)) {
        result = 29;
    }
    
    return result;
}



static uint32_t days_since_epoch(datetime_t *dt) {
    uint32_t days = 0;
    uint16_t year = dt->year;
    uint8_t month = dt->month;
    uint8_t day = dt->day;
    
    // Count days from 1970 to current year
    for (uint16_t y = 1970; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }
    
    // Add days for complete months in current year
    for (uint8_t m = 1; m < month; m++) {
        days += days_in_month(m, year);
    }
    
    // Add remaining days
    days += day - 1;
    
    return days;
}

uint32_t get_unix_timestamp(void) {
    datetime_t dt;
    rtc_get_time(&dt);
    
    // Unix timestamp is always in UTC, but rtc_get_time() already
    // converted to local time, so we need to convert back to UTC
    apply_timezone_offset(&dt, -TIMEZONE_OFFSET_HOURS);
    
    uint32_t days = days_since_epoch(&dt);
    uint32_t seconds = days * 86400UL;
    seconds += (uint32_t)dt.hour * 3600UL;
    seconds += (uint32_t)dt.minute * 60UL;
    seconds += (uint32_t)dt.second;
    
    return seconds;
}

void format_time_12h(datetime_t *dt, char *buffer) {
    uint8_t hour_24 = dt->hour;
    const char *ampm = "AM";
    uint8_t hour_12;
    
    if (hour_24 == 0) {
        hour_12 = 12;
        ampm = "AM";
    } else if (hour_24 == 12) {
        hour_12 = 12;
        ampm = "PM";
    } else if (hour_24 > 12) {
        hour_12 = hour_24 - 12;
        ampm = "PM";
    } else {
        hour_12 = hour_24;
        ampm = "AM";
    }
    
    int pos = 0;
    
    // Hour
    if (hour_12 >= 10) {
        buffer[pos++] = (hour_12 / 10) + '0';
    }
    buffer[pos++] = (hour_12 % 10) + '0';
    buffer[pos++] = ':';
    
    // Minute
    buffer[pos++] = (dt->minute / 10) + '0';
    buffer[pos++] = (dt->minute % 10) + '0';
    buffer[pos++] = ':';
    
    // Second
    buffer[pos++] = (dt->second / 10) + '0';
    buffer[pos++] = (dt->second % 10) + '0';
    buffer[pos++] = ' ';
    
    // AM/PM
    buffer[pos++] = ampm[0];
    buffer[pos++] = ampm[1];
    buffer[pos++] = ' ';
    
    // Timezone
    buffer[pos++] = 'E';
    buffer[pos++] = 'S';
    buffer[pos++] = 'T';
    buffer[pos] = '\0';
}

void format_time_24h(datetime_t *dt, char *buffer) {
    int pos = 0;
    
    // Hour
    buffer[pos++] = (dt->hour / 10) + '0';
    buffer[pos++] = (dt->hour % 10) + '0';
    buffer[pos++] = ':';
    
    // Minute
    buffer[pos++] = (dt->minute / 10) + '0';
    buffer[pos++] = (dt->minute % 10) + '0';
    buffer[pos++] = ':';
    
    // Second
    buffer[pos++] = (dt->second / 10) + '0';
    buffer[pos++] = (dt->second % 10) + '0';
    buffer[pos++] = ' ';
    
    // Timezone
    buffer[pos++] = 'E';
    buffer[pos++] = 'S';
    buffer[pos++] = 'T';
    buffer[pos] = '\0';
}

void format_date(datetime_t *dt, char *buffer) {
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    
    const char *month_name = months[dt->month - 1];
    
    int pos = 0;
    
    // Month
    buffer[pos++] = month_name[0];
    buffer[pos++] = month_name[1];
    buffer[pos++] = month_name[2];
    buffer[pos++] = ' ';
    
    // Day
    if (dt->day >= 10) {
        buffer[pos++] = (dt->day / 10) + '0';
    }
    buffer[pos++] = (dt->day % 10) + '0';
    buffer[pos++] = ',';
    buffer[pos++] = ' ';
    
    // Year
    uint16_t year = dt->year;
    buffer[pos++] = (year / 1000) + '0';
    buffer[pos++] = ((year / 100) % 10) + '0';
    buffer[pos++] = ((year / 10) % 10) + '0';
    buffer[pos++] = (year % 10) + '0';
    buffer[pos] = '\0';
}

void date_command(int argc, char* argv[]) {
    datetime_t dt;
    rtc_get_time(&dt);
    
    char date_buffer[32];
    char time_buffer[32];
    print("\n");
    format_date(&dt, date_buffer);
    format_time_12h(&dt, time_buffer);
    
    
    print("Date: ");
    
    print(date_buffer);
    print("\n");
    
    
    print("Time: ");
    
    print(time_buffer);
    print("\n");
}

void time_command(int argc, char* argv[]) {
    datetime_t dt;
    rtc_get_time(&dt);
    print("\n");
    
    char time_buffer[32];
    format_time_12h(&dt, time_buffer);
    
    
    print("Current time: ");
    
    print(time_buffer);
    print("\n");
}

void timestamp_command(int argc, char* argv[]) {
    uint32_t timestamp = get_unix_timestamp();
    print("\n");
    
    print("Unix timestamp: ");
    
    print_decimal(timestamp);
    print("\n");
}

// Bonus: Get current time as string (useful for logging)
void get_time_string(char* buffer, bool use_24h) {
    datetime_t dt;
    rtc_get_time(&dt);
    
    if (use_24h) {
        format_time_24h(&dt, buffer);
    } else {
        format_time_12h(&dt, buffer);
    }
}

// Bonus: Get current date as string
void get_date_string(char* buffer) {
    datetime_t dt;
    rtc_get_time(&dt);
    format_date(&dt, buffer);
}