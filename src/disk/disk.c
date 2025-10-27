#include "disk.h"
#include "../vga/vga.h"
#include "../clib/clib.h"

int disk_read_lba(uint32_t lba, void* buffer);
int disk_write_lba(uint32_t lba, const void* buffer);

/* Compatibility wrappers for legacy functions */
int disk_read_sector(uint32_t lba, void* buffer) {
    return disk_read_lba(lba, buffer);
}

int disk_write_sector(uint32_t lba, const void* buffer) {
    return disk_write_lba(lba, buffer);
}

void disk_init(void) {
    vga_putstr("disk: ATA driver ready\n", 0x0A);
}
