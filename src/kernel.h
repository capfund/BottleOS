#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

// Dark mode color codes
#define DARK_WHITE_ON_BLACK 0x0F
#define DARK_GREEN_ON_BLACK 0x02

// Light mode color codes
#define LIGHT_WHITE_ON_BLACK 0xF0
#define LIGHT_GREEN_ON_BLACK 0xF2

// Global theme flag
extern int light_mode;

// Functions to get current colors
static inline uint8_t color_white_on_black() {
    return light_mode ? LIGHT_WHITE_ON_BLACK : DARK_WHITE_ON_BLACK;
}

static inline uint8_t color_green_on_black() {
    return light_mode ? LIGHT_GREEN_ON_BLACK : DARK_GREEN_ON_BLACK;
}

#endif
