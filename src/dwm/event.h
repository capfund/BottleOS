// Minimal raw input event queue
#ifndef DWM_EVENT_H
#define DWM_EVENT_H

#include <stdint.h>

#define EVENT_QUEUE_SIZE 256

typedef enum {
    EVENT_NONE = 0,
    EVENT_KEY,        // raw scancode
    EVENT_MOUSE_MOVE, // relative dx/dy
    EVENT_MOUSE_BTN,  // button press/release
} EventType;

typedef struct {
    EventType type;
    union {
        struct { unsigned char scancode; } key;
        struct { int dx; int dy; unsigned char left; unsigned char right; unsigned char middle; } mouse;
    } u;
} InputEvent;

void event_init(void);
int  event_push(const InputEvent *ev);
int  event_pop(InputEvent *out);

#endif
