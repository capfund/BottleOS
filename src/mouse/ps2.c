#include <stdint.h>
#include <stddef.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

typedef struct {
    int8_t dx;
    int8_t dy;
    int8_t dz;       // scroll wheel (always 0 in this 3-byte-only driver)
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

/* Bounded flush: empties up to MAX_FLUSH bytes from controller to avoid infinite loops */
static void ps2_flush_input(void) {
    const int MAX_FLUSH = 64;
    int n = 0;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) && n++ < MAX_FLUSH) {
        (void)inb(PS2_DATA_PORT);
    }
}

/* Write a byte to the mouse (via 0xD4). */
static int mouse_write_byte(uint8_t val) {
    if (ps2_wait_input_clear() < 0) return -1;
    outb(PS2_CMD_PORT, 0xD4);
    if (ps2_wait_input_clear() < 0) return -1;
    outb(PS2_DATA_PORT, val);
    return 0;
}

/* Read a byte from mouse with timeout */
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

/* Mouse initialization - keep simple and tolerant */
int mouse_init(void) {
    ps2_flush_input();

    /* Reset mouse and wait for ACK + BAT (0xAA) */
    if (mouse_send_command_with_ack(0xFF) != PS2_ACK) return -1;

    /* After reset, the mouse sends BAT (0xAA) and then device ID (0x00) usually.
       Read until we find 0xAA or timeout (be tolerant of ordering differences). */
    int found = 0;
    for (int i = 0; i < 8; ++i) {
        int b = mouse_read_timeout();
        if (b < 0) break;
        if (b == 0xAA) { found = 1; break; }
    }
    if (!found) return -2;

    /* Flush any leftover bytes before continuing */
    ps2_flush_input();

    /* Set defaults and enable streaming (standard 3-byte behavior) */
    if (mouse_send_command_with_ack(0xF6) != PS2_ACK) return -4;
    if (mouse_send_command_with_ack(0xF4) != PS2_ACK) return -5;

    /* Final flush */
    ps2_flush_input();
    return 0;
}

/* =========================
   mouse_poll - STRICT 3-BYTE
   =========================
   - Forces 3-byte packets only (no 4th wheel byte).
   - Resynchronizes robustly if first byte invalid.
   - Discards packets where X/Y overflow bits are set (bits 6/7).
*/
int mouse_poll(MousePacket *out) {
    static uint8_t buf[3];
    static int index = 0;

    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OBF) {
        uint8_t b = inb(PS2_DATA_PORT);

        /* If we're at packet start, first byte must have bit 3 set (0x08).
           If not, we consider it noise and continue (resync). */
        if (index == 0) {
            if (!(b & 0x08)) {
                index = 0;
                continue;
            }
            buf[0] = b;
            index = 1;
            continue;
        }

        /* store subsequent bytes */
        buf[index++] = b;

        if (index >= 3) {
            /* We have 3 bytes -> validate and produce packet */
            index = 0;

            /* If overflow bits (X or Y overflow) are set in first byte, discard packet */
            /* bits 6 (X overflow) and 7 (Y overflow) */
            if (buf[0] & 0xC0) {
                /* drop this packet and continue reading new ones */
                continue;
            }

            /* decode */
            int8_t dx = (int8_t)buf[1];
            int8_t dy = (int8_t)buf[2];

            out->dx = dx;
            out->dy = dy;
            out->dz = 0; /* wheel disabled in strict 3-byte mode */

            out->left   = buf[0] & 0x01;
            out->right  = (buf[0] >> 1) & 1;
            out->middle = (buf[0] >> 2) & 1;

            return 1;
        }
    }

    return 0;
}
