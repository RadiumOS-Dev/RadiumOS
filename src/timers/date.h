#ifndef TIME_H
#define TIME_H

#include <stdint.h>

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} datetime_t;

int rtc_init(void);
void rtc_get_time(datetime_t *dt);
uint32_t get_unix_timestamp(void);
void format_time_12h(datetime_t *dt, char *buffer);
void format_time_24h(datetime_t *dt, char *buffer);
void format_date(datetime_t *dt, char *buffer);
void timestamp_command(int argc, char* argv[]);
void time_command(int argc, char* argv[]);
void date_command(int argc, char* argv[]);
#endif
