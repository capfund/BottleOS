BITS 32
section .multiboot
align 4
MULTIBOOT_MAGIC     equ 0x1BADB002
MULTIBOOT_FLAGS     equ 0x00000003  ; Enable memory info + VBE info
MULTIBOOT_CHECKSUM  equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS  
dd MULTIBOOT_CHECKSUM

; VBE info structure
dd 0    ; header_addr
dd 0    ; load_addr  
dd 0    ; load_end_addr
dd 0    ; bss_end_addr
dd 0    ; entry_addr
dd 0    ; mode_type (0 = linear graphics)
dd 1024 ; width
dd 768  ; height  
dd 32   ; depth

section .text
global _start
extern kernel_main

_start:
    ; Multiboot provides EAX=magic, EBX=info
    push ebx
    push eax
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang