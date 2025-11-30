#include <stdint.h>
#include <stddef.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

typedef struct {
    int8_t dx;
    int8_t dy;
    int8_t dz;       // scroll wheel
    uint8_t left;
    uint8_t right;
    uint8_t middle;
} MousePacket;

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_CMD_PORT     0x64

#define PS2_STATUS_OBF   0x01
#define PS2_STATUS_IBF   0x02

#define PS2_ACK          0xFA
#define PS2_RESEND       0xFE

static const int TIMEOUT = 100000;
static const int COMMAND_RETRIES = 3;

/* Low-level helpers */
int mouse_try_read_byte(void) {
    if (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OBF)) return -1;
    return inb(PS2_DATA_PORT);
}

static int ps2_wait_input_clear(void) {
    int t = TIMEOUT;
    while (t-- && (inb(PS2_STATUS_PORT) & PS2_STATUS_IBF));
    return (t <= 0) ? -1 : 0;
}

static int ps2_wait_output_full(void) {
    int t = TIMEOUT;
    while (t-- && !(inb(PS2_STATUS_PORT) & PS2_STATUS_OBF));
    return (t <= 0) ? -1 : 0;
}

static void ps2_flush_input(void) {
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OBF)
        (void)inb(PS2_DATA_PORT);
}

static int mouse_write_byte(uint8_t val) {
    if (ps2_wait_input_clear() < 0) return -1;
    outb(PS2_CMD_PORT, 0xD4);
    if (ps2_wait_input_clear() < 0) return -1;
    outb(PS2_DATA_PORT, val);
    return 0;
}

static int mouse_read_timeout(void) {
    if (ps2_wait_output_full() < 0) return -1;
    return inb(PS2_DATA_PORT);
}

static int mouse_send_command_with_ack(uint8_t cmd) {
    for (int attempt = 0; attempt < COMMAND_RETRIES; ++attempt) {
        if (mouse_write_byte(cmd) < 0) return -1;
        int resp = mouse_read_timeout();
        if (resp < 0) return -1;
        if (resp == PS2_ACK) return PS2_ACK;
        if (resp == PS2_RESEND) continue;
        return resp;
    }
    return -1;
}

/* Mouse initialization */
int mouse_init(void) {
    ps2_flush_input();

    if (mouse_send_command_with_ack(0xFF) != PS2_ACK) return -1;

    int byte = mouse_read_timeout();
    if (byte < 0) return -2;
    if (byte != 0xAA) {
        int next = mouse_read_timeout();
        if (next != 0xAA) return -3;
    }

    ps2_flush_input();

    if (mouse_send_command_with_ack(0xF6) != PS2_ACK) return -4;
    if (mouse_send_command_with_ack(0xF4) != PS2_ACK) return -5;

    ps2_flush_input();
    return 0;
}

/* Polling and packet parser with 3- or 4-byte packets */
int mouse_poll(MousePacket *out) {
    static unsigned char buf[4];
    static int index = 0;
    static int packet_size = 3; // default 3, auto-detect if 4-byte

    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) {
        unsigned char b = inb(PS2_DATA_PORT);

        /* First byte must have bit 3 set */
        if (index == 0 && !(b & 0x08)) {
            index = 0;
            continue;
        }

        buf[index++] = b;

        if (index == 3 && (buf[0] & 0x08)) {
            /* Check if mouse has scroll wheel (4-byte packet) */
            if (buf[0] & 0x08) packet_size = 4; // most modern mice default 4-byte
        }

        if (index >= packet_size) {
            index = 0;

            int8_t dx = (int8_t)buf[1];
            int8_t dy = (int8_t)buf[2];
            int8_t dz = 0;

            if (packet_size == 4) dz = (int8_t)buf[3];

            out->dx = dx;
            out->dy = dy;
            out->dz = dz;
            out->left   = buf[0] & 0x01;
            out->right  = (buf[0] & 0x02) >> 1;
            out->middle = (buf[0] & 0x04) >> 2;

            return 1;
        }
    }

    return 0;
}
