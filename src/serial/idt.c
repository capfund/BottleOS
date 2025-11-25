#include <stdint.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20

/* IDT entry (32-bit) */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
//static struct idt_ptr   idtp;

/* External ISR stubs defined in assembly */
extern void isr_default_stub(void);
extern void isr_serial_stub(void); /* IRQ4 handler stub */

/* I/O helpers */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Load IDT */
static inline void lidt(void* base, uint16_t size) {
    struct idt_ptr ptr;
    ptr.limit = size - 1;
    ptr.base  = (uint32_t)base;
    asm volatile ("lidt %0" : : "m"(ptr));
}

/* Set an IDT gate */
void idt_set_gate(int n, uint32_t handler_addr, uint16_t sel, uint8_t flags) {
    idt[n].offset_low  = handler_addr & 0xFFFF;
    idt[n].selector    = sel;
    idt[n].zero        = 0;
    idt[n].type_attr   = flags;
    idt[n].offset_high = (handler_addr >> 16) & 0xFFFF;
}

/* Remap the PIC so IRQs start at 0x20 (32) */
void pic_remap(void) {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    outb(PIC1_DATA, 0x20); /* Master PIC vector offset */
    outb(PIC2_DATA, 0x28); /* Slave PIC vector offset */
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* restore saved masks */
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

/* Initialize IDT with a default stub and leave specific vectors replaceable
void idt_init(void) {
    int i;
    pic_remap();

    for(i = 0; i < 256; ++i)
        idt_set_gate(i, (uint32_t)isr_default_stub, 0x08, 0x8E);

    lidt(idt, sizeof(idt));
} */

void idt_init(void) {
    int i;

    pic_remap();

    for(i = 0; i < 256; ++i)
        idt_set_gate(i, (uint32_t)isr_default_stub, 0x08, 0x8E);

    /* Install serial IRQ handler */
    idt_set_gate(0x24, (uint32_t)isr_serial_stub, 0x08, 0x8E);

    lidt(idt, sizeof(idt));
}

/* Enable a specific IRQ line on the PIC (0..15) */
void enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t mask;

    if(irq < 8) {
        port = PIC1_DATA;
        mask = inb(port);
        mask &= ~(1 << irq);
        outb(port, mask);
    } else {
        port = PIC2_DATA;
        mask = inb(port);
        mask &= ~(1 << (irq - 8));
        outb(port, mask);
    }
}

/* Disable a specific IRQ */
void disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t mask;

    if(irq < 8) {
        port = PIC1_DATA;
        mask = inb(port);
        mask |= (1 << irq);
        outb(port, mask);
    } else {
        port = PIC2_DATA;
        mask = inb(port);
        mask |= (1 << (irq - 8));
        outb(port, mask);
    }
}

/* Send End-of-Interrupt to PICs for the given IRQ number */
void send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

/* Provide accessors for inb/outb to other C files */
uint8_t port_inb(uint16_t port) { return inb(port); }
void port_outb(uint16_t port, uint8_t val) { outb(port, val); }