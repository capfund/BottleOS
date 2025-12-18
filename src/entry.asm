BITS 32

section .multiboot
align 8

MB2_MAGIC      equ 0xE85250D6
MB2_ARCH_I386  equ 0
MB2_HEADER_LEN equ header_end - header_start
MB2_CHECKSUM  equ -(MB2_MAGIC + MB2_ARCH_I386 + MB2_HEADER_LEN)

header_start:
    dd MB2_MAGIC
    dd MB2_ARCH_I386
    dd MB2_HEADER_LEN
    dd MB2_CHECKSUM

    ; -----------------------------------------
    ; Framebuffer request tag
    ; type = 5, flags = 0 (optional)
    ; -----------------------------------------
    align 8
    dw 5                ; tag type
    dw 0                ; flags
    dd 20               ; size
    dd 1024             ; width
    dd 768              ; height
    dd 32               ; bpp

    ; -----------------------------------------
    ; End tag
    ; -----------------------------------------
    align 8
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    mov ebp, 0

    push ebx        ; multiboot2 info pointer
    push eax        ; magic
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
