; src/entry.asm
BITS 32

section .multiboot
align 4

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0x0
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; Multiboot header required by GRUB
dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

section .text
global _start
extern kernel_main
extern set_disk_module     ; we’ll define this in kernel.c

; Multiboot loads EBX = pointer to multiboot_info struct
_start:
    mov  eax, [esp + 8]    ; EAX = multiboot magic (not used)
    mov  ebx, [esp + 12]   ; EBX = multiboot info struct pointer
    push ebx
    call setup_disk_module  ; handle multiboot modules if any
    call kernel_main        ; jump into kernel

.hang:
    cli
    hlt
    jmp .hang


; --------------------------------------------------------
; Parse Multiboot modules and find disk.img (if provided)
; --------------------------------------------------------
setup_disk_module:
    push ebx
    mov  eax, [ebx + 20]   ; eax = mods_count
    cmp  eax, 0
    je   .no_modules       ; no modules → skip

    mov  ecx, [ebx + 24]   ; ecx = mods_addr (array of modules)
    mov  eax, [ecx]        ; eax = mod_start
    mov  edx, [ecx + 4]    ; edx = mod_end
    push edx
    push eax
    call set_disk_module   ; kernel.c will store these addresses
    add  esp, 8

.no_modules:
    pop ebx
    ret
