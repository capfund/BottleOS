#include "graphics.h"
#include "vga_driver.h"
#include "../mouse/ps2.h"
#include "../fs/fs.h"
#include "fonts.h"
#include "../clib/clib.h"

// Active driver
static graphics_driver_t *graphics_active_driver = 0;
extern void vga_flush();    

#define MAX_CLICKS 64

typedef struct { int x, y; } Click;
Click clicks[MAX_CLICKS];
int num_clicks = 0;

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
    graphics_plot_pixel(x, y, color);
    graphics_plot_pixel(x + 10, y, color);
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

static uint8_t reverse_bits(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void graphics_draw_char(int x, int y, char c, graphics_color_t color) {
    unsigned char uc = (unsigned char)c;
    if (uc < 32 || uc > 127) return;

    const uint8_t *bitmap = font8x8_basic[uc - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = reverse_bits(bitmap[row]);
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                graphics_plot_pixel(x + col, y + row, color);
            }
        }
    }
}

/* for future me: the color may use the rgb func below */
void graphics_draw_string(int x, int y, const char *str, graphics_color_t color) {
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 8;
            x = orig_x;
        } else {
            graphics_draw_char(x, y, *str, color);
            x += 8;
        }
        str++;
    }
}

/* For future me (params):
r8, g8, b8: rgb values
palette index: the palate to replace e.g replace 15 for white*/
uint8_t rgb(uint8_t r8, uint8_t g8, uint8_t b8, uint8_t palette_index) {
    // convert 0->255 to 0->63 (6bit channel)
    uint8_t r6 = r8 >> 2;
    uint8_t g6 = g8 >> 2;
    uint8_t b6 = b8 >> 2;

    outb(0x03C8, palette_index);
    outb(0x03C9, r6);            // r
    outb(0x03C9, g6);            // g
    outb(0x03C9, b6);            // b

    return palette_index; // return plotting index
}

void vga_test() {
    graphics_set_driver(&graphics_vga_driver);

    MousePacket pkt;
    if (mouse_init() != 0) {
        graphics_clear_screen();
        graphics_draw_rectangle(10, 50, 20, 20, 4); // error indicator
        while (1);
    }

    int mouse_x = 160;
    int mouse_y = 100;

    while (1) {
        // Clear screen
        graphics_clear_screen();

        // Draw static shapes
        graphics_draw_rectangle(150, 10, 100, 50, 3);
        graphics_draw_happy_face(10, 10, 2);
        graphics_draw_happy_face(100, 100, 5);
        graphics_draw_happy_face(200, 150, 5);
        graphics_draw_string(140, 90, "drawn in vga text mode", rgb(255,255,255,15));

        // Poll all mouse packets
        while (mouse_poll(&pkt) == 1) {
            mouse_x += pkt.dx;
            mouse_y -= pkt.dy;

            // Clamp to screen
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x > 319) mouse_x = 319;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y > 199) mouse_y = 199;

            // Left-click: record click position
            if (pkt.left != 0 && num_clicks < MAX_CLICKS) {
                clicks[num_clicks].x = mouse_x;
                clicks[num_clicks].y = mouse_y;
                num_clicks++;
            } else if (pkt.left != 0 && num_clicks >= MAX_CLICKS) {
                num_clicks = 0;
                clicks[num_clicks].x = mouse_x;
                clicks[num_clicks].y = mouse_y;
                num_clicks++;
            }
        }

        // Draw all recorded clicks
        for (int i = 0; i < num_clicks; i++) {
            graphics_draw_rectangle(clicks[i].x, clicks[i].y, 10, 10, 19);
        }

        // Draw cursor on top
        graphics_draw_cursor(mouse_x, mouse_y, 2);

        // Flush buffer to VGA
        vga_flush();
    }
}
