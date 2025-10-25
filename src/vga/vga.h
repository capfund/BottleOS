#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VIDEO_MEMORY ((char *)0xb8000)
#define VGA_MEM_WIDTH 80
#define VGA_MEM_HEIGHT 25

void vga_clear_screen(void);
void vga_putchar(char c, unsigned char color);
void vga_putstr(const char *str, unsigned char color);
unsigned int vga_get_cursor_row(void);
unsigned int vga_get_cursor_col(void);
void vga_set_cursor(unsigned int row, unsigned int col);

#endif
