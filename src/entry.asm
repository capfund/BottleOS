bits 32

section .multiboot
align 4
dd 0x1BADB002          ; Multiboot magic number
dd 0x0                 ; Flags
dd -(0x1BADB002 + 0x0) ; Checksum

section .bss
align 16
stack_bottom:
    resb 16384           ; 16 KB stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top   ; Setup stack
    call kernel_main     ; byebye botloader
halt:
    hlt
    jmp halt
