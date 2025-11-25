// src/serial.c
// Serial (COM1) mouse driver. C side of IRQ handling and UART init.
// Uses idt.c helpers for install.

#include <stdint.h>
#include <stddef.h>

/* External functions from idt.c */
extern void idt_set_gate(int n, uint32_t handler_addr, uint16_t sel, uint8_t flags);
extern void enable_irq(uint8_t irq);
extern void send_eoi(uint8_t irq);
extern void idt_init(void);
extern uint8_t port_inb(uint16_t port);
extern void port_outb(uint16_t port, uint8_t val);

// external functions from stubs
extern void isr_serial_stub();

/* IRQ mapping: PIC remapped base is 0x20 (32). COM1 is IRQ4 => vector = 0x20 + 4 = 0x24 */
#define IRQ_BASE       0x20
#define SERIAL_IRQ     4
#define SERIAL_VECTOR  (IRQ_BASE + SERIAL_IRQ)

#define COM1_PORT 0x3F8

/* small circular buffer */
#define RX_BUF_SIZE 256
static uint8_t rx_buf[RX_BUF_SIZE];
static size_t rx_in = 0, rx_out = 0;

static inline void enqueue(uint8_t b) {
    size_t next = (rx_in + 1) % RX_BUF_SIZE;
    if (next != rx_out) { rx_buf[rx_in] = b; rx_in = next; }
}

static inline int dequeue(void) {
    if (rx_out == rx_in) return -1;
    int c = rx_buf[rx_out];
    rx_out = (rx_out + 1) % RX_BUF_SIZE;
    return c;
}

/* Mouse packet processing (adapted from Chris Giese serial example) */
static uint8_t mouse_state = 0;
static uint8_t mouse_buf[4];
static int mouse_x = 40, mouse_y = 12;
static int screen_w = 80, screen_h = 25;

static void serial_mouse_process(uint8_t byte) {
    if (byte & 0x40) mouse_state = 0;
    if (mouse_state >= 4) return;
    mouse_buf[mouse_state++] = byte & 0x3F;

    if (mouse_state == 3) {
        int dx = ((mouse_buf[0]&0x03)<<6) | (mouse_buf[1]&0x3F);
        int dy = ((mouse_buf[0]&0x0C)<<4) | (mouse_buf[2]&0x3F);

        /* Sign-extend 8-bit as used in original code */
        if (dx & 0x80) dx -= 256;
        if (dy & 0x80) dy -= 256;

        int new_x = mouse_x + dx;
        int new_y = mouse_y + dy;
        if (new_x < 0) new_x = 0;
        if (new_x >= screen_w) new_x = screen_w - 1;
        if (new_y < 0) new_y = 0;
        if (new_y >= screen_h) new_y = screen_h - 1;

        mouse_x = new_x; mouse_y = new_y;

        /* (Optional) you can update a text-mode cursor here via direct video memory */
        mouse_state = 0;
    }
}

/* This is called by the assembly ISR stub (serial IRQ). Name matches EXTERN in isr_stub.asm */
void serial_irq_handler_c(void) {
    /* Read interrupt identification register and handle RX events.
       IIR: port+2. Data: port+0. */
    uint8_t iir;
    /* read IIR and loop while there are pending events (bit0==0 indicates pending) */
    while (((iir = port_inb(COM1_PORT + 2)) & 0x01) == 0) {
        uint8_t reason = (iir >> 1) & 0x07;
        if (reason == 2 || reason == 6) {
            /* Received data available / Character timeout => read byte */
            uint8_t b = port_inb(COM1_PORT + 0);
            enqueue(b);
            serial_mouse_process(b);
        } else {
            /* Other reasons: we will clear by reading other registers or just ignore */
            if (reason == 3) {
                /* Line status: read to clear */
                (void)port_inb(COM1_PORT + 5);
            } else if (reason == 1) {
                /* THR empty: read LSR or manage tx if needed (not used here) */
                (void)port_inb(COM1_PORT + 5);
            } else {
                /* modem status or other: read to clear */
                (void)port_inb(COM1_PORT + 6);
            }
        }
    }

    /* Send EOI */
    send_eoi(SERIAL_IRQ);
}

/* Initialize COM1 for serial mouse usage (1200, 7 bits typical for some serial mice).
   Does not enable interrupts on the CPU (caller must call sti after system IDT set). */
void serial_mouse_install(void) {
    /* Caller should call idt_init() once before calling this (or have an IDT already) */

    /* Hook the IDT vector to our assembly stub */
    //idt_set_gate(SERIAL_VECTOR, (uint32_t)(&isr_serial_stub), 0x08, 0x8E);
    idt_set_gate(SERIAL_VECTOR, (uint32_t)isr_serial_stub, 0x08, 0x8E);

    /* Configure UART */
    /* Disable interrupts */
    port_outb(COM1_PORT + 1, 0x00);
    /* Enable DLAB */
    port_outb(COM1_PORT + 3, 0x80);
    /* Set divisor for baud: for 1200, divisor = 115200 / 1200 = 96 */
    uint16_t divisor = 96;
    port_outb(COM1_PORT + 0, (uint8_t)(divisor & 0xFF)); /* DLL */
    port_outb(COM1_PORT + 1, (uint8_t)((divisor >> 8) & 0xFF)); /* DLM */
    /* 7 data bits, even/none depends on mouse; original used 7 for some mice; set 7 bits (0x02) or 8 bits (0x03).
       Using 7 bits like Chris Giese when MOUSE mode; adjust if needed. */
    port_outb(COM1_PORT + 3, 0x02); /* 7 bits, no parity, 1 stop (change to 0x03 for 8 bits) */
    /* Enable FIFO if supported and IRQs on RX */
    port_outb(COM1_PORT + 2, 0x0B); /* FIFO on, clear, 14-byte threshold; also sets interrupts to enabled for RX */
    /* Set modem control: RTS/DTR */
    port_outb(COM1_PORT + 4, 0x0B); /* RTS + DTR + AUX outputs */

    /* Unmask IRQ4 (COM1) on PIC */
    enable_irq(SERIAL_IRQ);

    /* At this point caller can safely `sti` to start receiving IRQs */
}

/* Helper to get latest mouse coordinates (non-blocking) */
void mouse_get_pos(int *x, int *y) {
    if (x) *x = mouse_x;
    if (y) *y = mouse_y;
}

// final helpers
int mouse_get_x(void) { return mouse_x; }
int mouse_get_y(void) { return mouse_y; }