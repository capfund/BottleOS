BITS 32

section .multiboot
align 4

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0x00000003
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

; framebuffer request (1024x768x32)
dd 1024
dd 768
dd 32

section .text
global _start
extern kernel_main

_start:
    ; EAX = multiboot magic
    ; EBX = multiboot info pointer
    push ebx
    push eax
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
