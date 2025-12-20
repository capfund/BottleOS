#include "vesa_driver.h"
#include "../clib/clib.h"

static uint32_t *framebuffer = 0;

static uint32_t fb_width  = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch  = 0;

graphics_buffer_t screen_buffer;
static graphics_buffer_t *current_target = 0;

static void vesa_set_target(graphics_buffer_t *buffer) {
    current_target = buffer;
}

static void vesa_init(void) {
    if (!framebuffer || fb_pitch == 0 || fb_height == 0)
        return;

    uint32_t bytes = fb_pitch * fb_height;
    uint32_t *bb = (uint32_t *)malloc(bytes);

    screen_buffer.pixels = bb;
    screen_buffer.width  = fb_width;
    screen_buffer.height = fb_height;
    screen_buffer.pitch  = fb_pitch;

    current_target = &screen_buffer;

    for (uint32_t i = 0; i < bytes / 4; i++)
        bb[i] = 0;
}

static void vesa_clear_screen(graphics_color_t color) {
    if (!current_target) return;

    for (uint32_t y = 0; y < current_target->height; y++) {
        uint32_t *row =
            (uint32_t *)((uint8_t *)current_target->pixels +
                         y * current_target->pitch);
        for (uint32_t x = 0; x < current_target->width; x++) {
            row[x] = color;
        }
    }
}

static void vesa_plot_pixel(int x, int y, graphics_color_t color) {
    if (!current_target) return;
    if (x < 0 || y < 0) return;
    if ((uint32_t)x >= current_target->width ||
        (uint32_t)y >= current_target->height)
        return;

    uint8_t *row =
        (uint8_t *)current_target->pixels + y * current_target->pitch;

    ((uint32_t *)row)[x] = color;
}

static void vesa_present(void) {
    uint32_t bytes = screen_buffer.pitch * screen_buffer.height;

    uint8_t *dst = (uint8_t *)framebuffer;
    uint8_t *src = (uint8_t *)screen_buffer.pixels;

    for (uint32_t i = 0; i < bytes; i++) {
        dst[i] = src[i];
    }
}

graphics_driver_t graphics_vesa_driver = {
    .init = vesa_init,
    .set_target = vesa_set_target,
    .clear_screen = vesa_clear_screen,
    .plot_pixel = vesa_plot_pixel,
    .present = vesa_present
};

void vesa_init_from_multiboot(
    uint64_t fb_addr,
    uint32_t width,
    uint32_t height,
    uint32_t pitch,
    uint8_t  bpp
) {
    if (bpp != 32) {
        for (;;);
    }

    framebuffer = (uint32_t *)(uint32_t)fb_addr;
    fb_width  = width;
    fb_height = height;
    fb_pitch  = pitch;
}
