#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include <stdint.h>

// 32-bit XRGB color
typedef uint32_t graphics_color_t;

typedef struct {
    uint32_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} graphics_buffer_t;

// Abstract graphics interface
typedef struct graphics_driver {
    void (*init)(void);
    void (*set_target)(graphics_buffer_t *buffer);
    void (*clear_screen)(graphics_color_t color);
    void (*plot_pixel)(int x, int y, graphics_color_t color);
    void (*present)(void);
} graphics_driver_t;

typedef struct {
    int x, y;
    int width, height;
    const char *label;
    graphics_color_t bg_color;
    graphics_color_t text_color;
    int text_scale;
    int border_rad;
} Button;

// Driver control
void graphics_set_driver(graphics_driver_t *driver);
void graphics_set_target(graphics_buffer_t *buffer);

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
void graphics_draw_rounded_rect(
    int x,
    int y,
    int width,
    int height,
    int radius,
    graphics_color_t color
);

// RGB helper
#define RGB(r,g,b) \
    ((graphics_color_t)(((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b)))

#endif
