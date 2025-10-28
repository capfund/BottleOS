#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

void display_putchar(char c, uint32_t color);
void display_putstr(const char *str, uint32_t color);
void display_clear(void);

#endif