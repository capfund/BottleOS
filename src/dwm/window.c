#include "window.h"
#include "../clib/clib.h"

void window_init(Window *w, int x, int y, int width, int height, const char *title, void (*draw)(void)) {
    w->x = x;
    w->y = y;
    w->width = width;
    w->height = height;
    w->title = title;
    w->owner = title;
    w->alive = 1;
    w->draw = draw;
    w->title = title;
    static unsigned int next_window_id = 1;
    w->id = next_window_id++;

    w->buffer.width  = width;
    w->buffer.height = height;
    w->buffer.pitch  = width * 4;
    size_t bytes = width * height * 4;
    w->buffer.pixels = malloc(bytes);
    if (w->buffer.pixels) {
        clib_account_alloc(w->owner, bytes);
    }
}

void window_destroy(Window *w) {
    if (!w) return;
    if (w->buffer.pixels) {
        size_t bytes = w->buffer.width * w->buffer.height * 4;
        clib_account_free(w->owner, bytes);
        free(w->buffer.pixels);
        w->buffer.pixels = NULL;
    }
    w->alive = 0;
}
