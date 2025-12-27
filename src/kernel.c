#include <stdint.h>
#include "multiboot2.h"
#include "graphics/graphics.h"
#include "graphics/vesa_driver.h"
#include "mouse/ps2.h"
#include "keyboard/keyboard.h"
#include "clib/clib.h"
#include <stdbool.h>
#include "dwm/dwm.h"
#include "fs/fs.h"

// compatibility reasons
int light_mode = 0;
#define MAX_CLICKS 64

typedef struct { int x, y; } Click;
Click clicks[MAX_CLICKS];
int num_clicks = 0;

bool clicked(const Button *btn, int mouse_x, int mouse_y) {
    return mouse_x >= btn->x &&
           mouse_x < (btn->x + btn->width) &&
           mouse_y >= btn->y &&
           mouse_y < (btn->y + btn->height);
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
    fs_init();
    dwm_init();

    for (;;) {
        dwm_frame();
    }
}
