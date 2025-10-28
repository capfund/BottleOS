#include "display.h"
#include "../vga/vga.h"
#include "../vesa/vesa.h"
#include "../kernel.h"

void display_putchar(char c, uint32_t color) {
    if (use_vesa) {
        vesa_term_putchar(c, color);
    } else {
        vga_putchar(c, (unsigned char)color);
    }
}

void display_putstr(const char *str, uint32_t color) {
    if (use_vesa) {
        vesa_term_putstr(str, color);
    } else {
        vga_putstr(str, (unsigned char)color);
    }
}

void display_clear(void) {
    if (use_vesa) {
        vesa_term_clear();
    } else {
        vga_clear_screen();
    }
}