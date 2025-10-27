#include "fs.h"
#include "../kernel.h"
#include "../vga/vga.h"
#include "../disk/disk.h"
#include <string.h>
#include <stddef.h>
#include <stdint.h>

/* In-memory state */
static fs_superblock_t superblock;
static fs_file_entry_t file_table[FS_MAX_FILES];
static uint32_t blocks_available = 0;

/* minimal kernel string helpers */
static void k_strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i], cb = (unsigned char)b[i];
        if (ca != cb)
            return (int)ca - (int)cb;
        if (ca == 0)
            return 0;
    }
    return 0;
}

/* ===== Disk-backed filesystem implementation ===== */

int fs_init(void) {
    uint8_t sector[FS_BLOCK_SIZE];
    if (disk_read_lba(0, sector) != 0) {
        vga_putstr("fs_init: disk read failed\n", 0x0C);
        return -1;
    }

    memcpy(&superblock, sector, sizeof(fs_superblock_t));

    if (superblock.magic != FS_MAGIC) {
        vga_putstr("fs: initializing fresh filesystem on disk\n", 0x0E);

        superblock.magic = FS_MAGIC;
        superblock.version = FS_VERSION;
        superblock.num_files = 0;
        superblock.max_files = FS_MAX_FILES;
        superblock.block_size = FS_BLOCK_SIZE;

        /* assume disk image large enough; use a safe fixed limit (e.g., 32768 blocks = 16MB) */
        superblock.total_blocks = 32768;

        uint32_t file_table_bytes = FS_MAX_FILES * sizeof(fs_file_entry_t);
        uint32_t file_table_blocks =
            (file_table_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        superblock.file_table_block = 1;
        superblock.data_block = 1 + file_table_blocks;

        /* write fresh superblock */
        memset(sector, 0, FS_BLOCK_SIZE);
        memcpy(sector, &superblock, sizeof(fs_superblock_t));
        disk_write_lba(0, sector);

        /* zero file table and data */
        memset(file_table, 0, sizeof(file_table));
        uint8_t zero[FS_BLOCK_SIZE];
        memset(zero, 0, FS_BLOCK_SIZE);
        for (uint32_t b = 0; b < file_table_blocks; b++) {
            disk_write_lba(superblock.file_table_block + b, zero);
        }
        /* (optional) zero data area if you want clean disk */
        vga_putstr("fs: filesystem created successfully\n", 0x0A);
    } else {
        vga_putstr("fs: found existing filesystem on disk\n", 0x0A);
        /* load file table into memory */
        uint32_t file_table_bytes = superblock.max_files * sizeof(fs_file_entry_t);
        uint32_t file_table_blocks =
            (file_table_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        uint8_t buf[FS_BLOCK_SIZE];
        for (uint32_t b = 0; b < file_table_blocks; b++) {
            disk_read_lba(superblock.file_table_block + b, buf);
            uint32_t offset = b * FS_BLOCK_SIZE;
            uint32_t copy_bytes = FS_BLOCK_SIZE;
            uint32_t remaining = file_table_bytes - offset;
            if (remaining < copy_bytes)
                copy_bytes = remaining;
            memcpy(((uint8_t *)file_table) + offset, buf, copy_bytes);
        }
    }

    blocks_available = superblock.total_blocks - superblock.data_block;
    return 0;
}

/* find file entry by name */
static fs_file_entry_t *find_entry(const char *name) {
    for (uint32_t i = 0; i < superblock.max_files; i++) {
        if (file_table[i].used &&
            k_strncmp(file_table[i].name, name, FS_FILENAME_LEN) == 0)
            return &file_table[i];
    }
    return NULL;
}

/* first-fit allocation */
static int allocate_blocks(uint32_t blocks_needed) {
    if (blocks_needed == 0)
        return -1;

    uint32_t total_data_blocks = superblock.total_blocks - superblock.data_block;
    for (uint32_t start = 0; start + blocks_needed <= total_data_blocks; start++) {
        uint8_t ok = 1;
        for (uint32_t j = 0; j < superblock.max_files; j++) {
            if (!file_table[j].used)
                continue;
            if (file_table[j].start_block == 0xFFFFFFFF)
                continue;

            uint32_t a = file_table[j].start_block;
            uint32_t b = a + ((file_table[j].size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE);
            uint32_t s1 = start, e1 = start + blocks_needed;
            uint32_t s2 = a, e2 = b;
            if (!(e1 <= s2 || e2 <= s1)) {
                ok = 0;
                break;
            }
        }
        if (ok)
            return (int)start;
    }
    return -1;
}

int fs_create_file(const char *name) {
    if (find_entry(name))
        return -2; /* exists */
    for (uint32_t i = 0; i < superblock.max_files; i++) {
        if (!file_table[i].used) {
            k_strncpy(file_table[i].name, name, FS_FILENAME_LEN);
            file_table[i].size = 0;
            file_table[i].start_block = 0xFFFFFFFF;
            file_table[i].used = 1;
            superblock.num_files++;
            return fs_sync();
        }
    }
    return -1;
}

int fs_write_file(const char *name, const uint8_t *data, uint32_t size) {
    fs_file_entry_t *e = find_entry(name);
    if (!e) {
        if (fs_create_file(name) < 0)
            return -1;
        e = find_entry(name);
        if (!e)
            return -1;
    }

    if (e->start_block != 0xFFFFFFFF && e->size > 0) {
        e->start_block = 0xFFFFFFFF;
        e->size = 0;
    }

    if (size == 0) {
        e->size = 0;
        e->start_block = 0xFFFFFFFF;
        return fs_sync();
    }

    uint32_t blocks_needed = (size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    int start = allocate_blocks(blocks_needed);
    if (start < 0) {
        vga_putstr("fs: no contiguous space\n", 0x0C);
        return -2;
    }

    uint32_t block_idx = (uint32_t)start;
    uint32_t bytes_written = 0;
    uint8_t sector[FS_BLOCK_SIZE];
    for (uint32_t bi = 0; bi < blocks_needed; bi++) {
        uint32_t to_copy = size - bytes_written;
        if (to_copy > FS_BLOCK_SIZE)
            to_copy = FS_BLOCK_SIZE;
        memset(sector, 0, FS_BLOCK_SIZE);
        memcpy(sector, data + bytes_written, to_copy);
        disk_write_lba(superblock.data_block + block_idx + bi, sector);
        bytes_written += to_copy;
    }

    e->start_block = block_idx;
    e->size = size;
    return fs_sync();
}

int fs_read_file(const char *name, uint8_t *buf, uint32_t bufsize) {
    fs_file_entry_t *e = find_entry(name);
    if (!e)
        return -1;
    if (bufsize < e->size)
        return -2;
    if (e->start_block == 0xFFFFFFFF)
        return 0;

    uint8_t sector[FS_BLOCK_SIZE];
    uint32_t block_idx = e->start_block;
    uint32_t bytes_read = 0;
    uint32_t remaining = e->size;
    uint32_t total_blocks = (e->size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;

    for (uint32_t bi = 0; bi < total_blocks; bi++) {
        disk_read_lba(superblock.data_block + block_idx + bi, sector);
        uint32_t to_copy = remaining > FS_BLOCK_SIZE ? FS_BLOCK_SIZE : remaining;
        memcpy(buf + bytes_read, sector, to_copy);
        bytes_read += to_copy;
        remaining -= to_copy;
    }

    return bytes_read;
}

int fs_delete_file(const char *name) {
    fs_file_entry_t *e = find_entry(name);
    if (!e)
        return -1;
    e->used = 0;
    e->size = 0;
    e->start_block = 0xFFFFFFFF;
    if (superblock.num_files > 0)
        superblock.num_files--;
    return fs_sync();
}

void fs_list_files(void) {
    for (uint32_t i = 0; i < superblock.max_files; i++) {
        if (file_table[i].used) {
            vga_putstr(file_table[i].name, 0x0F);
            vga_putstr(" (", 0x0F);
            char dec[12];
            uint32_t s = file_table[i].size;
            int pos = 0;
            if (s == 0)
                dec[pos++] = '0';
            else {
                char rev[12];
                int r = 0;
                while (s) {
                    rev[r++] = '0' + (s % 10);
                    s /= 10;
                }
                while (r--)
                    dec[pos++] = rev[r];
            }
            dec[pos] = '\0';
            vga_putstr(dec, 0x0F);
            vga_putstr(")\n", 0x0F);
        }
    }
}

int fs_sync(void) {
    uint8_t sector[FS_BLOCK_SIZE];
    memset(sector, 0, FS_BLOCK_SIZE);
    memcpy(sector, &superblock, sizeof(fs_superblock_t));
    if (disk_write_lba(0, sector) != 0)
        return -1;

    uint32_t file_table_bytes = superblock.max_files * sizeof(fs_file_entry_t);
    uint32_t file_table_blocks =
        (file_table_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;

    for (uint32_t b = 0; b < file_table_blocks; b++) {
        uint32_t offset = b * FS_BLOCK_SIZE;
        uint32_t to_copy = FS_BLOCK_SIZE;
        if (offset + to_copy > file_table_bytes)
            to_copy = file_table_bytes - offset;
        memset(sector, 0, FS_BLOCK_SIZE);
        memcpy(sector, ((uint8_t *)file_table) + offset, to_copy);
        if (disk_write_lba(superblock.file_table_block + b, sector) != 0)
            return -1;
    }
    return 0;
}
