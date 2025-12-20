#ifndef __BLIT_H
#define __BLIT_H

#include "../graphics/graphics.h"

void blit_buffer(
    graphics_buffer_t *src,
    graphics_buffer_t *dst,
    int dx,
    int dy
);

#endif
