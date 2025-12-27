#include "isr.h"
#include "../clib/clib.h"

#define IDT_ENTRIES 256

static idt_entry_t idt[IDT_ENTRIES];
static void (*irq_routines[16])(void);

// assembly functions
extern void load_idt(uint32_t);
extern void isr_stub_table(void); 

static void idt_set_gate(int n, uint32_t handler, uint16_t sel, uint8_t flags) {
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].selector = sel;
    idt[n].zero = 0;
    idt[n].type_attr = flags;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void isr_handler_c(int irq_number) {
    // push to kb handler if irq=1 (code from kb)
    if (irq_number == 1) {
        extern void keyboard_irq_handler(void);
        keyboard_irq_handler();
    }

    // irq handlers below
}

void isr_init(void) {
    // Initialize PIC
    outb(0x20, 0x11); // Master
    outb(0xA0, 0x11); // Slave
    outb(0x21, 0x20); // Master offset
    outb(0xA1, 0x28); // Slave offset
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    // Initialize IDT entries
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint32_t)isr_stub_table + i * 8, 0x08, 0x8E);
    }

    idtr_t idtr;
    idtr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idtr.base = (uint32_t)&idt;
    load_idt((uint32_t)&idtr);
}

void irq_install_handler(int irq, void (*handler)(void)) {
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq) {
    irq_routines[irq] = 0;
}

void irq_handler(int irq) {
    if (irq_routines[irq])
        irq_routines[irq]();

    // Send EOI to PIC
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void isr_enable_interrupts(void) { __asm__ volatile("sti"); }
void isr_disable_interrupts(void) { __asm__ volatile("cli"); }
