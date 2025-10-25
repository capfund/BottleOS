#include "keyboard.h"
#include "../clib/clib.h"

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int caps_lock_on = 0;

void keyboard_handle_modifier(unsigned char scancode) {
  switch (scancode) {
  case 0x2A:
    shift_pressed = 1;
    break; // Left Shift
  case 0x36:
    shift_pressed = 1;
    break; // Right Shift
  case 0xAA:
    shift_pressed = 0;
    break; // Left Shift release
  case 0xB6:
    shift_pressed = 0;
    break; // Right Shift release
  case 0x1D:
    ctrl_pressed = 1;
    break; // Left Ctrl
  case 0x9D:
    ctrl_pressed = 0;
    break; // Left Ctrl release
  case 0x3A:
    caps_lock_on ^= 1;
    break; // Caps Lock toggle
  }
}

char keyboard_scancode_to_ascii(unsigned char scancode) {
  static char normal[128] = {
      0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
      '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
      'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
      'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
      'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0};

  static char shifted[128] = {
      0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
      '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
      'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
      'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
      'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' ', 0};

  if (scancode >= 128)
    return 0;

  char c = normal[scancode];

  if ((c >= 'a' && c <= 'z')) {
    if ((shift_pressed && !caps_lock_on) || (!shift_pressed && caps_lock_on))
      c -= 32; // uppercase letters
  } else if (shift_pressed) {
    c = shifted[scancode]; // shifted symbols
  }

  return c;
}

unsigned char keyboard_get_scancode() {
  while (!(inb(KEYBOARD_STATUS_PORT) & 1)) {
  }
  return inb(KEYBOARD_DATA_PORT);
}

int keyboard_is_shift_pressed(void) { return shift_pressed; }
int keyboard_is_ctrl_pressed(void) { return ctrl_pressed; }
int keyboard_is_caps_lock_on(void) { return caps_lock_on; }
