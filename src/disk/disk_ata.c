#include "disk.h"
#include "../vga/vga.h"
#include "../clib/clib.h"
#include <stdint.h>
#include <stddef.h>

#define ATA_PRIMARY_IO   0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_RDY  0x40
#define ATA_STATUS_DRQ  0x08

static void io_wait(void) {
    for (volatile int i = 0; i < 1000; i++);
}

/* We’ll use inb/outb from clib.h — no need to redefine them */

static void insw(uint16_t port, void* addr, uint32_t count) {
    __asm__ volatile ("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

static void outsw(uint16_t port, const void* addr, uint32_t count) {
    __asm__ volatile ("rep outsw" : "+S"(addr), "+c"(count) : "d"(port));
}

static int ata_wait_bsy(void) {
    while (inb(ATA_PRIMARY_IO + 7) & ATA_STATUS_BSY);
    return 0;
}

static int ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_IO + 7) & ATA_STATUS_DRQ));
    return 0;
}

int disk_read_lba(uint32_t lba, void* buffer) {
    ata_wait_bsy();

    outb(ATA_PRIMARY_IO + 6, 0xE0 | ((lba >> 24) & 0x0F)); // drive/head
    outb(ATA_PRIMARY_IO + 2, 1);                           // sector count
    outb(ATA_PRIMARY_IO + 3, (uint8_t)lba);
    outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_IO + 7, 0x20);                        // READ SECTORS

    ata_wait_bsy();
    ata_wait_drq();

    insw(ATA_PRIMARY_IO, buffer, 256); // 512 bytes
    io_wait();
    return 0;
}

int disk_write_lba(uint32_t lba, const void* buffer) {
    ata_wait_bsy();

    outb(ATA_PRIMARY_IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + 2, 1);
    outb(ATA_PRIMARY_IO + 3, (uint8_t)lba);
    outb(ATA_PRIMARY_IO + 4, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_IO + 5, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_IO + 7, 0x30);                        // WRITE SECTORS

    ata_wait_bsy();
    ata_wait_drq();

    outsw(ATA_PRIMARY_IO, buffer, 256);
    io_wait();

    return 0;
}
