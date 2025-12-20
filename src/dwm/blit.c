#include "blit.h"

void blit_buffer(
    graphics_buffer_t *src,
    graphics_buffer_t *dst,
    int dx,
    int dy
) {
    for (uint32_t y = 0; y < src->height; y++) {
        uint32_t dy2 = dy + y;
        if (dy2 >= dst->height) continue;

        uint32_t *srow =
            (uint32_t *)((uint8_t *)src->pixels + y * src->pitch);
        uint32_t *drow =
            (uint32_t *)((uint8_t *)dst->pixels + dy2 * dst->pitch);

        for (uint32_t x = 0; x < src->width; x++) {
            uint32_t dx2 = dx + x;
            if (dx2 >= dst->width) continue;
            drow[dx2] = srow[x];
        }
    }
}
