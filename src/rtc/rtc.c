#include "rtc.h"
#include "../clib/clib.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

// CMOS registers
#define CMOS_SECONDS   0x00
#define CMOS_MINUTES   0x02
#define CMOS_HOURS     0x04
#define CMOS_DAY       0x07
#define CMOS_MONTH     0x08
#define CMOS_YEAR      0x09
#define CMOS_STATUS_A  0x0A
#define CMOS_STATUS_B  0x0B

// ------------------------------
// Helpers
// ------------------------------
static int rtc_is_updating(void) {
    outb(CMOS_ADDR, CMOS_STATUS_A);
    return inb(CMOS_DATA) & 0x80; // bit 7
}

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

static uint8_t bcd_to_bin(uint8_t val) {
    return (val & 0x0F) + ((val >> 4) * 10);
}

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) ||
           (year % 400 == 0);
}

// Days per month
static const int days_in_month[] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

// ------------------------------
// Public API
// ------------------------------
void rtc_read(struct rtc_time *t) {
    uint8_t regB;

    // Wait until RTC is stable
    while (rtc_is_updating());

    t->second = cmos_read(CMOS_SECONDS);
    t->minute = cmos_read(CMOS_MINUTES);
    t->hour   = cmos_read(CMOS_HOURS);
    t->day    = cmos_read(CMOS_DAY);
    t->month  = cmos_read(CMOS_MONTH);
    t->year   = cmos_read(CMOS_YEAR);

    regB = cmos_read(CMOS_STATUS_B);

    // Convert BCD to binary if needed
    if (!(regB & 0x04)) {
        t->second = bcd_to_bin(t->second);
        t->minute = bcd_to_bin(t->minute);
        t->hour   = bcd_to_bin(t->hour);
        t->day    = bcd_to_bin(t->day);
        t->month  = bcd_to_bin(t->month);
        t->year   = bcd_to_bin(t->year);
    }

    // Convert 12-hour to 24-hour format
    if (!(regB & 0x02) && (t->hour & 0x80)) {
        t->hour = ((t->hour & 0x7F) + 12) % 24;
    }

    // Assume century = 2000+
    t->year += 2000;
}

uint64_t rtc_to_unix(const struct rtc_time *t) {
    uint64_t days = 0;

    // Years since 1970
    for (int year = 1970; year < t->year; year++) {
        days += is_leap_year(year) ? 366 : 365;
    }

    // Months in current year
    for (int month = 1; month < t->month; month++) {
        days += days_in_month[month - 1];
        if (month == 2 && is_leap_year(t->year)) {
            days += 1;
        }
    }

    // Days in current month
    days += t->day - 1;

    // Convert to seconds
    return (days * 86400ULL) +
           (t->hour * 3600ULL) +
           (t->minute * 60ULL) +
           t->second;
}
