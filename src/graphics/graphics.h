#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include <stdint.h>

// Color type
typedef uint8_t graphics_color_t;

// Abstract graphics interface
typedef struct graphics_driver {
    void (*init)();
    void (*clear_screen)(graphics_color_t color);
    void (*plot_pixel)(int x, int y, graphics_color_t color);
} graphics_driver_t;

// High-level graphics functions
void graphics_draw_rectangle(int x, int y, int width, int height, graphics_color_t color);
void graphics_draw_happy_face(int x, int y, graphics_color_t color);

// Set the active graphics driver
void graphics_set_driver(graphics_driver_t *driver);

// Convenience wrapper functions
void graphics_clear_screen();
void graphics_plot_pixel(int x, int y, graphics_color_t color);
void vga_test();

#endif
