#pragma once
#include <stdint.h>

typedef struct {
    int8_t dx;
    int8_t dy;
    int8_t dz;       // scroll wheel
    uint8_t left;
    uint8_t right;
    uint8_t middle;
} MousePacket;

int mouse_try_read_byte(void);
int mouse_init(void);
int mouse_poll(MousePacket *out);
