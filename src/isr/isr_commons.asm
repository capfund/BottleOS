[BITS 32]
global isr_common_stub
extern isr_handler_c  ; C function to handle ISR

isr_common_stub:
    pusha                   ; save all registers
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10            ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push dword 0            ; placeholder for interrupt number if needed
    call isr_handler_c
    add esp, 4              ; remove interrupt number
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd

global load_idt
load_idt:
    mov eax, [esp+4] ; pointer to IDTR
    lidt [eax]
    ret
