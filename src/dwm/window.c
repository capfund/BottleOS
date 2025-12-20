#include "window.h"
#include "../clib/clib.h"

void window_init(Window *w, int x, int y, int width, int height, void (*draw)(void)) {
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->draw = draw;

    w->buffer.width  = width;
    w->buffer.height = height;
    w->buffer.pitch  = width * 4;
    w->buffer.pixels = malloc(width * height * 4);
}
