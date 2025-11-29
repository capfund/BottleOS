#include "graphics.h"
#include "vga_driver.h"

// Active driver
static graphics_driver_t *graphics_active_driver = 0;

void graphics_set_driver(graphics_driver_t *driver) {
    graphics_active_driver = driver;
    if (graphics_active_driver && graphics_active_driver->init) {
        graphics_active_driver->init();
    }
}

void graphics_clear_screen() {
    if (graphics_active_driver && graphics_active_driver->clear_screen) {
        graphics_active_driver->clear_screen(0); // default black
    }
}

void graphics_plot_pixel(int x, int y, graphics_color_t color) {
    if (graphics_active_driver && graphics_active_driver->plot_pixel) {
        graphics_active_driver->plot_pixel(x, y, color);
    }
}

void graphics_draw_rectangle(int x, int y, int width, int height, graphics_color_t color) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            graphics_plot_pixel(x + i, y + j, color);
        }
    }
}

void graphics_draw_happy_face(int x, int y, graphics_color_t color) {
    // eyes
    graphics_plot_pixel(x, y, color);
    graphics_plot_pixel(x + 10, y, color);

    // mouth
    for (int i = 0; i <= 8; i++) graphics_plot_pixel(x + i, y + 10, color);
    graphics_plot_pixel(x, y + 8, color);
    graphics_plot_pixel(x + 1, y + 9, color);
    graphics_plot_pixel(x + 9, y + 9, color);
    graphics_plot_pixel(x + 10, y + 8, color);
}

void vga_test() {
	graphics_set_driver(&graphics_vga_driver);

    // graphic functionalitys
    while (1) {
        graphics_clear_screen();
        graphics_draw_rectangle(150, 10, 100, 50, 3);
        // draw some faces
        graphics_draw_happy_face(10,10,2);
        graphics_draw_happy_face(100,100,5);
        graphics_draw_happy_face(300,150,5);
    }
}