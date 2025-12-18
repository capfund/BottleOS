#include "graphics.h"
#include "fonts.h"
#include "../clib/clib.h"

// Active driver
static graphics_driver_t *graphics_active_driver = 0;

void graphics_set_driver(graphics_driver_t *driver) {
    graphics_active_driver = driver;
    if (graphics_active_driver && graphics_active_driver->init) {
        graphics_active_driver->init();
    }
}

void graphics_clear_screen(graphics_color_t color) {
    if (graphics_active_driver && graphics_active_driver->clear_screen) {
        graphics_active_driver->clear_screen(color);
    }
}

void graphics_plot_pixel(int x, int y, graphics_color_t color) {
    if (graphics_active_driver && graphics_active_driver->plot_pixel) {
        graphics_active_driver->plot_pixel(x, y, color);
    }
}

void graphics_present(void) {
    if (graphics_active_driver && graphics_active_driver->present) {
        graphics_active_driver->present();
    }
}

void graphics_draw_rectangle(int x, int y, int width, int height, graphics_color_t color) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            graphics_plot_pixel(x + i, y + j, color);
        }
    }
}

void graphics_draw_happy_face(int x, int y, graphics_color_t color) {
    graphics_plot_pixel(x, y, color);
    graphics_plot_pixel(x + 10, y, color);
    for (int i = 0; i <= 8; i++)
        graphics_plot_pixel(x + i, y + 10, color);
}

void graphics_draw_cursor(int x, int y, graphics_color_t color) {
    for (int i = -4; i <= 4; i++) {
        graphics_plot_pixel(x + i, y, color);
        graphics_plot_pixel(x, y + i, color);
    }
}

void graphics_draw_char(int x, int y, char c, graphics_color_t color, int scale) {
    if (scale <= 0) scale = 1; // scale check

    uint8_t uc = (uint8_t)c;
    if (uc < 32 || uc > 127) return;

    const uint8_t *bitmap = font8x8_basic[uc];

    for (int row = 0; row < 8; row++) {
        uint8_t bits = bitmap[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << col)) {
                // Draw a square of size `scale x scale`
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        graphics_plot_pixel(x + col*scale + dx, y + row*scale + dy, color);
                    }
                }
            }
        }
    }
}


void graphics_draw_string(int x, int y, const char *str, graphics_color_t color, int scale) {
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 8 * scale;   // vert scale
            x = orig_x;
        } else {
            graphics_draw_char(x, y, *str, color, scale);
            x += 8 * scale;   // horizontal scale
        }
        str++;
    }
}

void draw_button(const Button *btn) {
    // Draw background
    graphics_draw_rectangle(btn->x, btn->y, btn->width, btn->height, btn->bg_color);

    // Draw label centered
    int label_len = 0;
    const char *s = btn->label;
    while (*s++) label_len++; // compute string length

    int scale = btn->text_scale > 0 ? btn->text_scale : 1;

    int text_width  = label_len * 8 * scale; // scaled width
    int text_height = 8 * scale;             // scaled height

    int text_x = btn->x + (btn->width - text_width) / 2;
    int text_y = btn->y + (btn->height - text_height) / 2;

    graphics_draw_string(text_x, text_y, btn->label, btn->text_color, scale);
}