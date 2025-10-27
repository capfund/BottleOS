#include "fs.h"
#include "../kernel.h" /* expects extern void *disk_module_addr; extern uint32_t disk_module_size; */
#include "../vga/vga.h"
#include <string.h> /* we implement minimal helpers below */

static fs_superblock_t superblock;
static fs_file_entry_t *file_table = NULL;
static uint8_t *data_area = NULL;
static uint32_t blocks_available = 0;

/* helper: safe strncpy-like for kernel (null-fill) */
static void k_strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i]; i++)
    dst[i] = src[i];
  for (; i < n; i++)
    dst[i] = '\0';
}

/* helper: compare up to n bytes */
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

/* low-level: map disk module into FS layout */
/* layout on the module:
   block 0 = superblock (512 bytes)
   block 1..N = file table (enough blocks as calculated)
   remaining blocks = data blocks
*/
int fs_init(void) {
  if (!disk_module_addr || disk_module_size < FS_BLOCK_SIZE) {
    vga_putstr("fs_init: no disk module loaded or too small\n", 0x0C);
    return -1;
  }

  uint8_t *mod = (uint8_t *)disk_module_addr;
  /* read superblock from block 0 */
  memcpy(&superblock, mod, sizeof(fs_superblock_t));

  if (superblock.magic != FS_MAGIC) {
    /* initialize a fresh filesystem layout on the image */
    vga_putstr("fs: initializing fresh filesystem on module\n", 0x0E);

    superblock.magic = FS_MAGIC;
    superblock.version = FS_VERSION;
    superblock.num_files = 0;
    superblock.max_files = FS_MAX_FILES;
    superblock.block_size = FS_BLOCK_SIZE;

    /* compute how many blocks we can use from the module */
    uint32_t total_blocks = (uint32_t)(disk_module_size / FS_BLOCK_SIZE);
    if (total_blocks < 4)
      return -1;
    superblock.total_blocks = total_blocks;

    /* reserve block 0 for superblock, file_table blocks after that.
       file table size = FS_MAX_FILES * sizeof(fs_file_entry_t)
    */
    uint32_t file_table_bytes = FS_MAX_FILES * sizeof(fs_file_entry_t);
    uint32_t file_table_blocks =
        (file_table_bytes + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    superblock.file_table_block = 1;
    superblock.data_block = 1 + file_table_blocks;

    /* write initial superblock into module memory */
    memcpy(mod, &superblock, sizeof(fs_superblock_t));

    /* zero-out file table area and data blocks */
    uint8_t *p = mod + superblock.file_table_block * FS_BLOCK_SIZE;
    uint32_t file_table_area = file_table_blocks * FS_BLOCK_SIZE;
    for (uint32_t i = 0; i < file_table_area; i++)
      p[i] = 0;
    /* zero data blocks */
    uint8_t *d = mod + superblock.data_block * FS_BLOCK_SIZE;
    uint32_t data_bytes =
        (total_blocks - superblock.data_block) * FS_BLOCK_SIZE;
    for (uint32_t i = 0; i < data_bytes; i++)
      d[i] = 0;
  } else {
    vga_putstr("fs: found existing filesystem on module\n", 0x0A);
  }

  /* compute pointers */
  blocks_available = superblock.total_blocks - superblock.data_block;
  file_table = (fs_file_entry_t *)((uint8_t *)disk_module_addr +
                                   superblock.file_table_block * FS_BLOCK_SIZE);
  data_area =
      (uint8_t *)disk_module_addr + superblock.data_block * FS_BLOCK_SIZE;

  return 0;
}

/* find file entry by name */
static fs_file_entry_t *find_entry(const char *name) {
  for (uint32_t i = 0; i < superblock.max_files; i++) {
    if (file_table[i].used &&
        k_strncmp(file_table[i].name, name, FS_FILENAME_LEN) == 0) {
      return &file_table[i];
    }
  }
  return NULL;
}

/* allocate contiguous blocks for size bytes. simple first-fit allocation */
static int allocate_blocks(uint32_t blocks_needed) {
  if (blocks_needed == 0)
    return -1;

  uint32_t total_data_blocks = superblock.total_blocks - superblock.data_block;
  for (uint32_t start = 0; start + blocks_needed <= total_data_blocks;
       start++) {
    uint8_t ok = 1;
    for (uint32_t j = 0; j < superblock.max_files; j++) {
      if (!file_table[j].used)
        continue;
      if (file_table[j].start_block == 0xFFFFFFFF)
        continue;

      uint32_t a = file_table[j].start_block;
      uint32_t b =
          a + ((file_table[j].size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE);
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
    return -2; /* already exists */
  for (uint32_t i = 0; i < superblock.max_files; i++) {
    if (!file_table[i].used) {
      k_strncpy(file_table[i].name, name, FS_FILENAME_LEN);
      file_table[i].size = 0;
      /* allocate zero blocks (start_block = 0 means none) */
      file_table[i].start_block = 0xFFFFFFFF;
      file_table[i].used = 1;
      superblock.num_files++;
      return 0;
    }
  }
  return -1; /* no space */
}

int fs_write_file(const char *name, const uint8_t *data, uint32_t size) {
  fs_file_entry_t *e = find_entry(name);
  if (!e) {
    // Create file if it doesn't exist
    if (fs_create_file(name) < 0)
      return -1;
    e = find_entry(name);
    if (!e)
      return -1;
  }

  // Free old blocks if file had data before
  if (e->start_block != 0xFFFFFFFF && e->size > 0) {
    /*
     * Mark old blocks as "freed" by setting start_block to invalid
     * The allocate_blocks function will skip files with start_block ==
     * 0xFFFFFFFF
     *
     */
    e->start_block = 0xFFFFFFFF;
    e->size = 0;
  }

  if (size == 0) {
    e->size = 0;
    e->start_block = 0xFFFFFFFF;
    return fs_sync();
  }

  // Compute needed blocks
  uint32_t blocks_needed = (size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
  int start = allocate_blocks(blocks_needed);
  if (start < 0) {
    vga_putstr("fs: no contiguous space available\n", 0x0C);
    return -2;
  }

  // Copy data into data area at (start)
  uint8_t *dest = data_area + start * FS_BLOCK_SIZE;
  for (uint32_t i = 0; i < size; i++)
    dest[i] = data[i];

  e->start_block = (uint32_t)start;
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
  uint8_t *src = data_area + e->start_block * FS_BLOCK_SIZE;
  for (uint32_t i = 0; i < e->size; i++)
    buf[i] = src[i];
  return (int)e->size;
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
  // char tmp[64];
  for (uint32_t i = 0; i < superblock.max_files; i++) {
    if (file_table[i].used) {
      /* print name and size */
      vga_putstr(file_table[i].name, 0x0F);
      vga_putstr(" (", 0x0F);
      /* convert size to decimal quickly */
      uint32_t s = file_table[i].size;
      int pos = 0;
      char dec[12];
      if (s == 0) {
        dec[pos++] = '0';
      } else {
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

/* Write the superblock and file table back into the disk module memory.
   This function modifies the module memory in-place, so it is visible
   for the running kernel. It does NOT write to the host disk file.
*/
int fs_sync(void) {
  if (!disk_module_addr)
    return -1;
  uint8_t *mod = (uint8_t *)disk_module_addr;
  /* write superblock to block 0 */
  memcpy(mod, &superblock, sizeof(fs_superblock_t));
  /* write file table */
  uint8_t *ft_dest = mod + superblock.file_table_block * FS_BLOCK_SIZE;
  memcpy(ft_dest, file_table, superblock.max_files * sizeof(fs_file_entry_t));
  /* If you also want to persist file data: each file already written directly
   * to data_area, which points into module memory */
  /* flush is a no-op — in many environments memory changes are already visible
   */
  return 0;
}
