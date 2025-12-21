#include "keyboard.h"
#include "../clib/clib.h"

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int caps_lock_on = 0;

void keyboard_handle_modifier(unsigned char scancode) {
    switch (scancode) {
        case 0x2A: case 0x36: shift_pressed = 1; break; // Shift press
        case 0xAA: case 0xB6: shift_pressed = 0; break; // Shift release
        case 0x1D: ctrl_pressed = 1; break;             // Ctrl press
        case 0x9D: ctrl_pressed = 0; break;             // Ctrl release
        case 0x3A: caps_lock_on ^= 1; break;           // Caps Lock toggle
    }
}

char keyboard_scancode_to_ascii(unsigned char scancode) {
    static const char normal[128] = {
        0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
        'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h',
        'j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0
    };

    static const char shifted[128] = {
        0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
        'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S','D','F','G','H',
        'J','K','L',':','"','~',0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',0
    };

    if (scancode >= 128) return 0;

    char c = normal[scancode];

    // Apply shift/caps logic
    if (c >= 'a' && c <= 'z') {
        if (shift_pressed ^ caps_lock_on) c -= 32; // XOR: only one active → uppercase
    } else if (shift_pressed) {
        c = shifted[scancode];
    }

    return c;
}

unsigned char keyboard_get_scancode(void) {
    for (int i = 0; i < 10000; i++) {  // small timeout
        uint8_t status = inb(KEYBOARD_STATUS_PORT);
        if (status & 1) {              // output buffer full
            if (status & 0x20) continue; // skip mouse
            return inb(KEYBOARD_DATA_PORT);
        }
    }
    return 0;
}

unsigned char keyboard_poll_scancode(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (!(status & 1)) return 0;
    if (status & 0x20) return 0;
    return inb(KEYBOARD_DATA_PORT);
}

int keyboard_is_shift_pressed(void) { return shift_pressed; }
int keyboard_is_ctrl_pressed(void)  { return ctrl_pressed; }
int keyboard_is_caps_lock_on(void)  { return caps_lock_on; }
