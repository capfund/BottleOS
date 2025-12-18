#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include <stdint.h>

// 32-bit XRGB color
typedef uint32_t graphics_color_t;

// Abstract graphics interface
typedef struct graphics_driver {
    void (*init)(void);
    void (*clear_screen)(graphics_color_t color);
    void (*plot_pixel)(int x, int y, graphics_color_t color);
    void (*present)(void);     // double-buffer flip
} graphics_driver_t;

typedef struct {
    int x, y;           // top-left corner
    int width, height;  // size
    const char *label;  // text
    graphics_color_t bg_color;
    graphics_color_t text_color;
    int text_scale; // docs: 1=normal size, 2=double size, etc.
} Button;

// Driver control
void graphics_set_driver(graphics_driver_t *driver);

// High-level drawing
void graphics_clear_screen(graphics_color_t color);
void graphics_plot_pixel(int x, int y, graphics_color_t color);
void graphics_present(void);

void graphics_draw_rectangle(int x, int y, int width, int height, graphics_color_t color);
void graphics_draw_happy_face(int x, int y, graphics_color_t color);
void graphics_draw_cursor(int x, int y, graphics_color_t color);
void graphics_draw_char(int x, int y, char c, graphics_color_t color, int scale);
void graphics_draw_string(int x, int y, const char *str, graphics_color_t color, int scale);
void draw_button(const Button *btn);

// RGB helper
#define RGB(r,g,b) \
    ((graphics_color_t)(((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b)))

#endif
