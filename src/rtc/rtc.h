#ifndef RTC_H
#define RTC_H

#include <stdint.h>

// RTC time structure
struct rtc_time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;   // full year (e.g. 2025)
};

// Read time from CMOS RTC (polling, no IRQs)
void rtc_read(struct rtc_time *t);

// Convert RTC time to Unix timestamp
uint64_t rtc_to_unix(const struct rtc_time *t);

#endif
