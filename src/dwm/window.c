#include "window.h"
#include "../clib/clib.h"

void window_init(Window *w, int x, int y, int width, int height, const char *title, void (*draw)(void)) {
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->title = title;
    w->alive = 1;
    w->draw = draw;
    w->title = title;

    w->buffer.width  = width;
    w->buffer.height = height;
    w->buffer.pitch  = width * 4;
    w->buffer.pixels = malloc(width * height * 4);
}

void window_destroy(Window *w) {
    if (!w) return;
    if (w->buffer.pixels) {
        free(w->buffer.pixels);
        w->buffer.pixels = NULL;
    }
    w->alive = 0;
}
