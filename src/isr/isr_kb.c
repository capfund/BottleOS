#include "isr.h"
#include "../clib/clib.h"       // for inb()
#include "../dwm/event.h"       // for InputEvent, event_push, EVENT_KEY
#include "../keyboard/keyboard.h"

void keyboard_irq_handler(void) {
    unsigned char sc = inb(KEYBOARD_DATA_PORT);

    InputEvent ev;
    ev.type = EVENT_KEY;
    ev.u.key.scancode = sc;
    event_push(&ev);

    keyboard_handle_modifier(sc);
}

// Initialize keyboard IRQ
void keyboard_init_irq(void) {
    irq_install_handler(1, keyboard_irq_handler); // IRQ1
}
