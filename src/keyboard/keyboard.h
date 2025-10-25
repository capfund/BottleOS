#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

void keyboard_handle_modifier(unsigned char scancode);
void keyboard_init(void);
unsigned char keyboard_get_scancode(void);
char keyboard_scancode_to_ascii(unsigned char scancode);
int keyboard_is_shift_pressed(void);
int keyboard_is_ctrl_pressed(void);
int keyboard_is_caps_lock_on(void);

#endif
