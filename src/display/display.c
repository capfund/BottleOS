#include "display.h"
#include "../vga/vga.h"
#include "../vesa/vesa.h"
#include "../kernel.h"

void display_putchar(char c, uint32_t color) {
    if (use_vesa) {
        vesa_term_putchar(c, color);
    } else {
        // Handle special characters for VGA
        if (c == '\b') {
            // Backspace: move cursor back and erase character
            unsigned int row = vga_get_cursor_row();
            unsigned int col = vga_get_cursor_col();
            
            if (col > 0) {
                vga_set_cursor(row, col - 1);
                vga_putchar(' ', 0x0F); // Erase with space
                vga_set_cursor(row, col - 1);
            } else if (row > 0) {
                vga_set_cursor(row - 1, VGA_MEM_WIDTH - 1);
                vga_putchar(' ', 0x0F); // Erase with space
                vga_set_cursor(row - 1, VGA_MEM_WIDTH - 1);
            }
        } else {
            vga_putchar(c, 0x0F); // White on black
        }
    }
}

void display_putstr(const char *str, uint32_t color) {
    if (use_vesa) {
        vesa_term_putstr(str, color);
    } else {
        // For VGA, handle the string character by character to process backspaces
        for (int i = 0; str[i] != '\0'; i++) {
            display_putchar(str[i], color);
        }
    }
}

void display_clear(void) {
    if (use_vesa) {
        vesa_term_clear();
    } else {
        vga_clear_screen();
    }
}