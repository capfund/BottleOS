#include "event.h"
#include "../clib/clib.h"

static InputEvent queue[EVENT_QUEUE_SIZE];
static int q_head = 0;
static int q_tail = 0;

void event_init(void) {
    q_head = q_tail = 0;
}

int event_push(const InputEvent *ev) {
    int next = (q_tail + 1) % EVENT_QUEUE_SIZE;
    if (next == q_head) return 0; // full
    queue[q_tail] = *ev;
    q_tail = next;
    return 1;
}

int event_pop(InputEvent *out) {
    if (q_head == q_tail) return 0; // empty
    *out = queue[q_head];
    q_head = (q_head + 1) % EVENT_QUEUE_SIZE;
    return 1;
}
