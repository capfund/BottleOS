#include <stdint.h>
#include "multiboot2.h"
#include "graphics/graphics.h"
#include "graphics/vesa_driver.h"
#include "mouse/ps2.h"

// compatibility reasons
int light_mode = 0;
#define MAX_CLICKS 64

typedef struct { int x, y; } Click;
Click clicks[MAX_CLICKS];
int num_clicks = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void kernel_main(uint32_t magic, uint32_t addr) {
    if (magic != MB2_MAGIC) {
        for (;;);
    }

    //mb2_info_t *mb2 = (mb2_info_t *)addr;
    mb2_tag_t *tag = (mb2_tag_t *)(addr + 8);

    mb2_tag_framebuffer_t *fb = 0;

    while (tag->type != 0) {
        if (tag->type == MB2_TAG_FRAMEBUFFER) {
            fb = (mb2_tag_framebuffer_t *)tag;
            break;
        }
        tag = (mb2_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    if (!fb || fb->framebuffer_type != 1) {
        for (;;); // No framebuffer → halt
    }

    vesa_init_from_multiboot(
        fb->addr,
        fb->width,
        fb->height,
        fb->pitch,
        fb->bpp
    );

    // tests and inits
    graphics_set_driver(&graphics_vesa_driver);

    MousePacket pkt;
    if (mouse_init() != 0) {
        for (;;); // halt (placeholder)
    }

    int mouse_x = 300;
    int mouse_y = 200;

    while (1) {
        graphics_clear_screen(RGB(0,12,255));
        graphics_draw_string(20, 20, "ever tried chicken?", RGB(255,255,255));

        while (mouse_poll(&pkt) == 1) {
            mouse_x += pkt.dx;
            mouse_y -= pkt.dy;

            // Clamp to screen
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x > 1023) mouse_x = 1023;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y > 767) mouse_y = 767;

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

        graphics_draw_cursor(mouse_x, mouse_y, RGB(255,255,255));

        graphics_present();
    }

}
