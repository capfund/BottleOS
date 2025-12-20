#ifndef __WINDOW_H
#define __WINDOW_H

#include "../graphics/graphics.h"

typedef struct {
    int x, y;
    int width, height;
    graphics_buffer_t buffer;
    void (*draw)(void);
} Window;

void window_init(Window *w, int x, int y, int width, int height, void (*draw)(void));

#endif
