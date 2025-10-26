#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>

#define FS_MAGIC 0x426F746C   /* "Botl" short magic */
#define FS_VERSION 1

#define FS_MAX_FILES 128
#define FS_FILENAME_LEN 32
#define FS_BLOCK_SIZE 512     /* bytes per block */
#define FS_MAX_BLOCKS 16384   /* safety limit */

typedef struct {
    char name[FS_FILENAME_LEN];
    uint32_t size;       /* bytes */
    uint32_t start_block;
    uint8_t used;        /* 0 = free, 1 = used */
    uint8_t reserved[3];
} fs_file_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t num_files;
    uint32_t max_files;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t file_table_block; /* block index where file table begins */
    uint32_t data_block;       /* block index where file data begins */
    uint8_t reserved[452];     /* pad to 512 bytes (superblock fits in one block) */
} fs_superblock_t;

/* public API */
int fs_init(void);
int fs_create_file(const char *name);
int fs_write_file(const char *name, const uint8_t *data, uint32_t size);
int fs_read_file(const char *name, uint8_t *buf, uint32_t bufsize);
int fs_delete_file(const char *name);
void fs_list_files(void);
int fs_sync(void); /* write metadata+table+data back to module memory (non-durable on host) */

#endif
