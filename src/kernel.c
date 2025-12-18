#include <stdint.h>
#include "multiboot2.h"

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

    uint32_t *framebuffer = (uint32_t *)(uint32_t)fb->addr;
    uint32_t pitch = fb->pitch / 4;

    uint32_t x = 10;
    uint32_t y = 10;

    framebuffer[y * pitch + x] = 0x00FF0000; // RED pixel (XRGB)

    for (;;)
        __asm__ volatile ("hlt");
}
