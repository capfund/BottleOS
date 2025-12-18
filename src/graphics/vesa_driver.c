#include "vesa_driver.h"

static uint32_t *framebuffer = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;

static void vesa_init(void) {
    // already initialized via multiboot
}

static void vesa_clear_screen(graphics_color_t color) {
    for (uint32_t y = 0; y < fb_height; y++) {
        uint32_t *row = (uint32_t *)((uint8_t *)framebuffer + y * fb_pitch);
        for (uint32_t x = 0; x < fb_width; x++) {
            row[x] = color;
        }
    }
}

static void vesa_plot_pixel(int x, int y, graphics_color_t color) {
    if (x < 0 || y < 0) return;
    if ((uint32_t)x >= fb_width || (uint32_t)y >= fb_height) return;

    uint32_t *pixel =
        (uint32_t *)((uint8_t *)framebuffer + y * fb_pitch) + x;

    *pixel = color;
}

graphics_driver_t graphics_vesa_driver = {
    .init = vesa_init,
    .clear_screen = vesa_clear_screen,
    .plot_pixel = vesa_plot_pixel
};

void vesa_init_from_multiboot(
    uint64_t fb_addr,
    uint32_t width,
    uint32_t height,
    uint32_t pitch,
    uint8_t  bpp
) {
    if (bpp != 32) {
        for (;;); // unsupported mode
    }

    framebuffer = (uint32_t *)(uint32_t)fb_addr;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;
}
