#pragma once
#include <stdint.h>

int disk_read_sector(uint32_t lba, void* buffer);
int disk_write_sector(uint32_t lba, const void* buffer);
void disk_init(void);

/* Modern ATA versions used by fs.c */
int disk_read_lba(uint32_t lba, void* buffer);
int disk_write_lba(uint32_t lba, const void* buffer);
