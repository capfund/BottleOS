; src/isr_stub.asm
; NASM ELF32 assembly. Provides:
;  - isr_default_stub: placeholder ISR that simply irets (safe)
;  - isr_serial_stub: ISR stub for serial IRQ (IRQ4) that:
;      * saves registers,
;      * sets DS/ES to kernel data selector (0x10),
;      * calls the C handler serial_irq_handler_c,
;      * restores registers, and irets.
;
; Build: nasm -f elf32 isr_stub.asm -o build/.../isr_stub.o

BITS 32
GLOBAL isr_default_stub
GLOBAL isr_serial_stub
EXTERN serial_irq_handler_c    ; C handler to call (defined in serial.c)

SECTION .text

; Default stub: just iret (used to populate IDT initially)
isr_default_stub:
    iret

; Serial IRQ stub
; Note: CPU has already pushed EIP, CS, EFLAGS on stack when jumping here.
isr_serial_stub:
    cli                 ; clear interrupts
    pusha               ; push general purpose registers (EAX..EDI)
    push ds
    push es

    ; set data segments to kernel data selector (assume 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    ; call the C handler. cdecl calling convention expects stack set.
    call serial_irq_handler_c

    pop es
    pop ds
    popa
    sti
    iret
