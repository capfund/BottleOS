#include "graphics.h"
#include "vga_driver.h"
#include "../mouse/ps2.h"
#include "../fs/fs.h"

// Active driver
static graphics_driver_t *graphics_active_driver = 0;
extern void vga_flush();    

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

void graphics_draw_cursor(int x, int y, graphics_color_t color) {
    for (int i = -2; i <= 2; i++) {
        graphics_plot_pixel(x + i, y, color);
        graphics_plot_pixel(x, y + i, color);
    }
}

void vga_test() {
    graphics_set_driver(&graphics_vga_driver);

    MousePacket pkt;
    if (mouse_init() != 0) {
        graphics_clear_screen();
        graphics_draw_rectangle(10, 50, 20, 20, 4); // error indicator
        while (1);
    }

    // Initial positions
    int mouse_x = 160;
    int mouse_y = 100;
    int scroll_bar_height = 20;

    while (1) {
        // Clear the screen once per frame
        graphics_clear_screen();

        // Draw static graphics
        graphics_draw_rectangle(150, 10, 100, 50, 3);
        graphics_draw_happy_face(10,10,2);
        graphics_draw_happy_face(100,100,5);
        graphics_draw_happy_face(200,150,5);

        // Poll all available mouse packets for this frame
        while (mouse_poll(&pkt) == 1) {
            mouse_x += pkt.dx;
            mouse_y -= pkt.dy;

            // Clamp to screen
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x > 319) mouse_x = 319;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y > 199) mouse_y = 199;

            // Scroll wheel visual
            if (pkt.dz != 0) {
                scroll_bar_height += pkt.dz * 2;
                if (scroll_bar_height < 5) scroll_bar_height = 5;
                if (scroll_bar_height > 50) scroll_bar_height = 50;
            }
            
            /* Left click demo (TBD)
            if (pkt.left != 0) {
                graphics_draw_rectangle(mouse_x, mouse_y, 10, 10, 19);
            } */
        }

        // Draw cursor and scroll bar once per frame
        graphics_draw_cursor(mouse_x, mouse_y, 2);
        graphics_draw_rectangle(300, 10, 20, scroll_bar_height, 5);
        vga_flush();
    }
}