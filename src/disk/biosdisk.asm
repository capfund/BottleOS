BITS 16
global bios_read_sector
global bios_write_sector

; BIOS read sector: int 0x13, ah=0x02
; void bios_read_sector(uint16_t drive, uint16_t cylinder, uint16_t head, uint16_t sector, void* buffer)
bios_read_sector:
    push bp
    mov bp, sp

    push ds
    push es
    mov bx, [bp + 12] ; buffer pointer (16-bit pointer in real mode)
    mov dl, [bp + 4]  ; drive
    mov ch, [bp + 6]  ; cylinder
    mov dh, [bp + 8]  ; head
    mov cl, [bp + 10] ; sector
    mov ah, 0x02
    mov al, 0x01      ; read one sector
    int 0x13

    pop es
    pop ds
    pop bp
    ret

; BIOS write sector: int 0x13, ah=0x03
bios_write_sector:
    push bp
    mov bp, sp

    push ds
    push es
    mov bx, [bp + 12]
    mov dl, [bp + 4]
    mov ch, [bp + 6]
    mov dh, [bp + 8]
    mov cl, [bp + 10]
    mov ah, 0x03
    mov al, 0x01
    int 0x13

    pop es
    pop ds
    pop bp
    ret
