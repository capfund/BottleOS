#include <stdint.h>
#include "multiboot2.h"
#include "graphics/graphics.h"
#include "graphics/vesa_driver.h"

// compatibility reasons
int light_mode = 0;

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

    graphics_set_driver(&graphics_vesa_driver);

    // Test
    graphics_clear_screen();
    graphics_draw_rectangle(50, 50, 200, 100, RGB(255,0,0));
    graphics_draw_string(60, 90, "hello CRUEL WORLD!!!", RGB(255,255,255));
}
