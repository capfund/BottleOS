#ifndef __VESA_DRIVER_H
#define __VESA_DRIVER_H

#include "graphics.h"
#include <stdint.h>

void vesa_init_from_multiboot(
    uint64_t fb_addr,
    uint32_t width,
    uint32_t height,
    uint32_t pitch,
    uint8_t  bpp
);

extern graphics_driver_t graphics_vesa_driver;

#endif
