#include "ps2.h"
#include <stdint.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_CMD_PORT     0x64

#define PS2_STATUS_OBF   0x01
#define PS2_STATUS_IBF   0x02

#define PS2_ACK          0xFA
#define PS2_RESEND       0xFE

static const int TIMEOUT = 100000;
static const int COMMAND_RETRIES = 3;

int mouse_try_read_byte(void) {
    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & PS2_STATUS_OBF)) return -1;   // no data
    if (!(status & 0x20)) return -1;             // not mouse
    return inb(PS2_DATA_PORT);
}

static int ps2_wait_input_clear(void) {
    int t = TIMEOUT;
    while (t-- && (inb(PS2_STATUS_PORT) & PS2_STATUS_IBF));
    return (t <= 0) ? -1 : 0;
}

static void ps2_flush_input(void) {
    int n = 0;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) && n++ < 64) {
        (void)inb(PS2_DATA_PORT);
    }
}

static int mouse_write_byte(uint8_t val) {
    if (ps2_wait_input_clear() < 0) return -1;
    outb(PS2_CMD_PORT, 0xD4);
    if (ps2_wait_input_clear() < 0) return -1;
    outb(PS2_DATA_PORT, val);
    return 0;
}

static int mouse_read_timeout(void) {
    int t = TIMEOUT;
    while (t-- && !(inb(PS2_STATUS_PORT) & PS2_STATUS_OBF));
    if (t <= 0) return -1;
    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & 0x20)) return -1; // skip keyboard
    return inb(PS2_DATA_PORT);
}

static int mouse_send_command_with_ack(uint8_t cmd) {
    for (int attempt = 0; attempt < COMMAND_RETRIES; ++attempt) {
        if (mouse_write_byte(cmd) < 0) return -1;
        int resp = mouse_read_timeout();
        if (resp < 0) return -1;
        if (resp == PS2_ACK) return PS2_ACK;
        if (resp == PS2_RESEND) continue;
    }
    return -1;
}

int mouse_init(void) {
    ps2_flush_input();
    if (mouse_send_command_with_ack(0xFF) != PS2_ACK) return -1;

    int found = 0;
    for (int i = 0; i < 8; ++i) {
        int b = mouse_read_timeout();
        if (b < 0) break;
        if (b == 0xAA) { found = 1; break; }
    }
    if (!found) return -2;

    ps2_flush_input();
    if (mouse_send_command_with_ack(0xF6) != PS2_ACK) return -4;
    if (mouse_send_command_with_ack(0xF4) != PS2_ACK) return -5;

    ps2_flush_input();
    return 0;
}

int mouse_poll(MousePacket *out) {
    static uint8_t buf[3];
    static int index = 0;
    int dx = 0, dy = 0;
    uint8_t left=0, right=0, middle=0;
    int packets = 0;

    while (1) {
        int b = mouse_try_read_byte();
        if (b < 0) break; // no more bytes

        uint8_t byte = (uint8_t)b;

        if (index == 0) {
            if (!(byte & 0x08)) continue; // resync
            buf[0] = byte;
            index = 1;
            continue;
        }

        buf[index++] = byte;

        if (index < 3) continue;

        index = 0;
        if (buf[0] & 0xC0) continue; // drop overflow

        dx += (int8_t)buf[1];
        dy += (int8_t)buf[2];
        left   |= buf[0] & 0x01;
        right  |= (buf[0] >> 1) & 1;
        middle |= (buf[0] >> 2) & 1;

        packets++;
    }

    if (packets == 0) return 0;

    out->dx = dx;
    out->dy = dy;
    out->dz = 0;
    out->left = left;
    out->right = right;
    out->middle = middle;

    return 1;
}
