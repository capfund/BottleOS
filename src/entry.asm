BITS 32
section .multiboot
align 4

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0x00000003      ; bit 2 = request framebuffer, bits 0+1 = required
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

; request framebuffer fields (1024x768 fb)
dd 1024            ; framebuffer_width
dd 768             ; framebuffer_height
dd 32              ; framebuffer_bpp

_start:
    ; Multiboot provides EAX=magic, EBX=info
    push ebx
    push eax
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
