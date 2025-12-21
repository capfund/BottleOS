#ifndef __WINDOW_H
#define __WINDOW_H

#include "../graphics/graphics.h"

typedef struct {
    int x, y;
    int width, height;
    graphics_buffer_t buffer;
    const char *title;
    int alive;
    void (*draw)(void);
    unsigned int id; // stable identifier for the window instance
} Window;

void window_init(Window *w, int x, int y, int width, int height, const char *title, void (*draw)(void));
void window_destroy(Window *w);

#endif
