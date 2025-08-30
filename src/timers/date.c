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

#define EST_OFFSET_HOURS 5  // EST is UTC-5

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

int rtc_init(void) {
    // Check if RTC is available
    if (rtc_update_in_progress()) {
        return 0; // RTC busy, initialization failed
    }
    
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    
    // Set 12-hour format (clear bit 1)
    status_b &= ~0x02;  // 12-hour format
    status_b |= 0x10;   // Binary mode (disable BCD)
    
    rtc_write(RTC_STATUS_B, status_b);
    
    // Verify the settings were applied
    uint8_t verify = rtc_read(RTC_STATUS_B);
    if ((verify & 0x10) == 0) {
        return 0; // Failed to set binary mode
    }
    
    return 1; // Success
}

void rtc_get_time(datetime_t *dt) {
    while (rtc_update_in_progress());

    dt->second = rtc_read(RTC_SECONDS);
    dt->minute = rtc_read(RTC_MINUTES);
    dt->hour = rtc_read(RTC_HOURS) + 1;
    dt->day = rtc_read(RTC_DAY);
    dt->month = rtc_read(RTC_MONTH);
    dt->year = rtc_read(RTC_YEAR);

    uint8_t status_b = rtc_read(RTC_STATUS_B);

    // Convert from BCD if necessary
    if (!(status_b & 0x04)) {
        dt->second = bcd_to_bin(dt->second);
        dt->minute = bcd_to_bin(dt->minute);
        dt->hour = bcd_to_bin(dt->hour);
        dt->day = bcd_to_bin(dt->day);
        dt->month = bcd_to_bin(dt->month);
        dt->year = bcd_to_bin(dt->year);
    }

    // Handle 12-hour format (bit 1 clear means 12-hour)
    if (!(status_b & 0x02)) {
        // In 12-hour mode, bit 7 of hour indicates PM
        if (dt->hour & 0x80) {
            dt->hour = (dt->hour & 0x7F);
            if (dt->hour != 12) {
                dt->hour += 12;
            }
        } else if (dt->hour == 12) {
            dt->hour = 0; // 12 AM = 0 hours
        }
    }

    dt->year += 2000;
    
    // Convert UTC to EST (subtract 5 hours)
    if (dt->hour >= EST_OFFSET_HOURS) {
        dt->hour -= EST_OFFSET_HOURS;
    } else {
        dt->hour = 24 - EST_OFFSET_HOURS + dt->hour;
        // Would need to adjust date as well for complete accuracy
        if (dt->day > 1) {
            dt->day--;
        } else {
            // Handle month/year rollback - simplified version
            if (dt->month > 1) {
                dt->month--;
                // Set to last day of previous month (simplified)
                dt->day = 30; // Approximation
            } else {
                dt->month = 12;
                dt->day = 31;
                dt->year--;
            }
        }
    }
}

static int is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static uint32_t days_since_epoch(datetime_t *dt) {
    uint32_t days = 0;
    uint16_t year = dt->year;
    uint8_t month = dt->month;
    uint8_t day = dt->day;

    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    for (uint16_t y = 1970; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }

    for (uint8_t m = 1; m < month; m++) {
        days += days_in_month[m - 1];
        if (m == 2 && is_leap_year(year)) {
            days++;
        }
    }

    days += day - 1;
    return days;
}

uint32_t get_unix_timestamp(void) {
    datetime_t dt;
    rtc_get_time(&dt);
    
    // Add EST offset back for Unix timestamp (which is UTC)
    dt.hour += EST_OFFSET_HOURS;
    if (dt.hour >= 24) {
        dt.hour -= 24;
        dt.day++;
        // Handle day/month/year overflow (simplified)
    }
    
    uint32_t days = days_since_epoch(&dt);
    uint32_t seconds = days * 86400;
    seconds += dt.hour * 3600;
    seconds += dt.minute * 60;
    seconds += dt.second;
    
    return seconds;
}

void format_time_12h(datetime_t *dt, char *buffer) {
    uint8_t hour_12 = dt->hour;
    const char *ampm = "AM";
    
    if (hour_12 == 0) {
        hour_12 = 12;
    } else if (hour_12 == 12) {
        ampm = "PM";
    } else if (hour_12 > 12) {
        hour_12 -= 12;
        ampm = "PM";
    }
    
    buffer[0] = (hour_12 / 10) + '0';
    buffer[1] = (hour_12 % 10) + '0';
    buffer[2] = ':';
    buffer[3] = (dt->minute / 10) + '0';
    buffer[4] = (dt->minute % 10) + '0';
    buffer[5] = ':';
    buffer[6] = (dt->second / 10) + '0';
    buffer[7] = (dt->second % 10) + '0';
    buffer[8] = ' ';
    buffer[9] = ampm[0];
    buffer[10] = ampm[1];
    buffer[11] = ' ';
    buffer[12] = 'E';
    buffer[13] = 'S';
    buffer[14] = 'T';
    buffer[15] = '\0';
}

void format_time_24h(datetime_t *dt, char *buffer) {
    buffer[0] = (dt->hour / 10) + '0';
    buffer[1] = (dt->hour % 10) + '0';
    buffer[2] = ':';
    buffer[3] = (dt->minute / 10) + '0';
    buffer[4] = (dt->minute % 10) + '0';
    buffer[5] = ':';
    buffer[6] = (dt->second / 10) + '0';
    buffer[7] = (dt->second % 10) + '0';
    buffer[8] = ' ';
    buffer[9] = 'E';
    buffer[10] = 'S';
    buffer[11] = 'T';
    buffer[12] = '\0';
}

void format_date(datetime_t *dt, char *buffer) {
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    
    const char *month_name = months[dt->month - 1];
    
    int pos = 0;
    buffer[pos++] = month_name[0];
    buffer[pos++] = month_name[1];
    buffer[pos++] = month_name[2];
    buffer[pos++] = ' ';
    buffer[pos++] = (dt->day / 10) + '0';
    buffer[pos++] = (dt->day % 10) + '0';
    buffer[pos++] = ',';
    buffer[pos++] = ' ';
    
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
    
    char date_buffer[20];
    char time_buffer[16];
    
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
    
    char time_buffer[16];
    format_time_12h(&dt, time_buffer);
    
    print("Current time: ");
    print(time_buffer);
    print("\n");
}

void timestamp_command(int argc, char* argv[]) {
    uint32_t timestamp = get_unix_timestamp();
    print("Unix timestamp: ");
    print_decimal(timestamp);
    print("\n");
}
