#include "fs.h"
#include "../disk/disk.h"
#include "../kernel.h"
#include "../vga/vga.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../clib/clib.h"

/* In-memory state */
static fs_superblock_t superblock;
static fs_file_entry_t file_table[FS_MAX_FILES];
static uint32_t blocks_available = 0;
static char current_directory[FS_FILENAME_LEN] = "/";

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

/* Validate that a stored filename contains only printable ASCII (32 .. 126).
 * Treat empty names or names with invalid bytes as unusable. */
static int is_valid_filename(const char *n) {
  if (!n || n[0] == '\0')
    return 0;
  for (size_t i = 0; i < FS_FILENAME_LEN && n[i]; i++) {
    unsigned char c = (unsigned char)n[i];
    if (c < 32 || c > 126)
      return 0;
  }
  return 1;
}

/* ===== Disk-backed filesystem implementation ===== */

int fs_init(void) {
  uint8_t sector[FS_BLOCK_SIZE];
  if (disk_read_lba(0, sector) != 0) {
    vga_putstr("fs_init: disk read failed\n", 0x0C);
    return -1;
  }

  memcpy(&superblock, sector, sizeof(fs_superblock_t));

  /* Ensure in-memory file table is zeroed to avoid leftover garbage */
  memset(file_table, 0, sizeof(file_table));
  if (superblock.magic != FS_MAGIC) {
    vga_putstr("fs: initializing fresh filesystem on disk\n", 0x0E);

    superblock.magic = FS_MAGIC;
    superblock.version = FS_VERSION;
    superblock.num_files = 0;
    superblock.max_files = FS_MAX_FILES;
    superblock.block_size = FS_BLOCK_SIZE;

    /* assume disk image large enough; use a safe fixed limit (e.g., 32768
     * blocks = 16MB) */
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

/* find file entry by name, considering current directory */
static fs_file_entry_t *find_entry(const char *name) {
  char full_path[FS_FILENAME_LEN * 2];

  // If name starts with /, it's absolute
  if (name[0] == '/') {
    k_strncpy(full_path, name + 1, FS_FILENAME_LEN); // skip the /
  }
  // If we're in root, just use the name
  else if (strcmp(current_directory, "/") == 0) {
    k_strncpy(full_path, name, FS_FILENAME_LEN);
  }
  // Otherwise prepend current directory
  else {
    int i = 0, j = 0;
    // Copy current dir
    while (current_directory[i] && i < FS_FILENAME_LEN - 1) {
      full_path[j++] = current_directory[i++];
    }
    // Add separator if needed
    if (j > 0 && full_path[j - 1] != '/') {
      full_path[j++] = '/';
    }
    // Copy name
    i = 0;
    while (name[i] && j < FS_FILENAME_LEN * 2 - 1) {
      full_path[j++] = name[i++];
    }
    full_path[j] = '\0';
  }

  /* Now search for the full path, but ignore entries with invalid names */
  for (uint32_t i = 0; i < superblock.max_files; i++) {
    if (!file_table[i].used)
      continue;
    if (!is_valid_filename(file_table[i].name))
      continue;
    if (k_strncmp(file_table[i].name, full_path, FS_FILENAME_LEN) == 0)
      return &file_table[i];
  }
  return NULL;
}

/* first-fit allocation */
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
  char full_path[FS_FILENAME_LEN * 2];

  // Build full path
  if (name[0] == '/') {
    k_strncpy(full_path, name + 1, FS_FILENAME_LEN);
  } else if (strcmp(current_directory, "/") == 0) {
    k_strncpy(full_path, name, FS_FILENAME_LEN);
  } else {
    int i = 0, j = 0;
    while (current_directory[i] && i < FS_FILENAME_LEN - 1) {
      full_path[j++] = current_directory[i++];
    }
    if (j > 0 && full_path[j - 1] != '/') {
      full_path[j++] = '/';
    }
    i = 0;
    while (name[i] && j < FS_FILENAME_LEN * 2 - 1) {
      full_path[j++] = name[i++];
    }
    full_path[j] = '\0';
  }

  if (find_entry(name))
    return -2; /* exists */
  for (uint32_t i = 0; i < superblock.max_files; i++) {
    if (!file_table[i].used) {
      k_strncpy(file_table[i].name, full_path, FS_FILENAME_LEN);
      file_table[i].size = 0;
      file_table[i].start_block = 0xFFFFFFFF;
      file_table[i].used = 1;
      file_table[i].is_directory = 0;
      superblock.num_files++;
      return fs_sync();
    }
  }
  return -1;
}

int fs_create_directory(const char *name) {
  char full_path[FS_FILENAME_LEN * 2];

  // Build full path (same logic as create_file)
  if (name[0] == '/') {
    k_strncpy(full_path, name + 1, FS_FILENAME_LEN);
  } else if (strcmp(current_directory, "/") == 0) {
    k_strncpy(full_path, name, FS_FILENAME_LEN);
  } else {
    int i = 0, j = 0;
    while (current_directory[i] && i < FS_FILENAME_LEN - 1) {
      full_path[j++] = current_directory[i++];
    }
    if (j > 0 && full_path[j - 1] != '/') {
      full_path[j++] = '/';
    }
    i = 0;
    while (name[i] && j < FS_FILENAME_LEN * 2 - 1) {
      full_path[j++] = name[i++];
    }
    full_path[j] = '\0';
  }

  if (find_entry(name))
    return -2; /* exists */
  for (uint32_t i = 0; i < superblock.max_files; i++) {
    if (!file_table[i].used) {
      k_strncpy(file_table[i].name, full_path, FS_FILENAME_LEN);
      file_table[i].size = 0;
      file_table[i].start_block = 0xFFFFFFFF;
      file_table[i].used = 1;
      file_table[i].is_directory = 1; // Mark as directory
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
    // no contiguous space code -2 err
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
    int prefix_len = 0;
    char prefix[FS_FILENAME_LEN];

    // Build prefix based on current directory
    if (strcmp(current_directory, "/") != 0) {
        k_strncpy(prefix, current_directory, FS_FILENAME_LEN);
        prefix_len = 0;
        while (prefix[prefix_len])
            prefix_len++;
        if (prefix_len > 0 && prefix[prefix_len - 1] != '/') {
            prefix[prefix_len++] = '/';
            prefix[prefix_len] = '\0';
        }
    }

    for (uint32_t i = 0; i < superblock.max_files; i++) {
        if (!file_table[i].used)
            continue;
        if (!is_valid_filename(file_table[i].name))
            continue;

        char *display_name = file_table[i].name;

        // Determine display_name relative to current directory
        if (prefix_len > 0) {
            if (k_strncmp(file_table[i].name, prefix, prefix_len) != 0)
                continue; // Skip files not in this directory
            display_name = file_table[i].name + prefix_len; // Show name without prefix
        } else {
            // In root, skip files with / in the name (they're in subdirs)
            int has_slash = 0;
            for (int j = 0; file_table[i].name[j]; j++) {
                if (file_table[i].name[j] == '/') {
                    has_slash = 1;
                    break;
                }
            }
            if (has_slash)
                continue;
        }

        // Skip entries named "@" or "2" completely
        if (strcmp(display_name, "@") == 0 || strcmp(display_name, "2") == 0)
            continue;

        // Print directory or file
        if (file_table[i].is_directory) {
            vga_putstr("[DIR] ", 0x0B);
        } else {
            vga_putstr("      ", 0x0F);
        }

        vga_putstr(display_name, 0x0F);

        // Print file size for non-directories
        if (!file_table[i].is_directory) {
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
            vga_putstr(" bytes)", 0x0F);
        }

        vga_putchar('\n', 0x0F);
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

int fs_delete_directory(const char *name) {
  fs_file_entry_t *e = find_entry(name);
  if (!e)
    return -1;
  if (!e->is_directory)
    return -2; // Not a directory

  e->used = 0;
  e->size = 0;
  e->start_block = 0xFFFFFFFF;
  e->is_directory = 0;
  if (superblock.num_files > 0)
    superblock.num_files--;
  return fs_sync();
}

int fs_is_directory(const char *name) {
  fs_file_entry_t *e = find_entry(name);
  if (!e)
    return -1;
  return e->is_directory ? 1 : 0;
}

const char *fs_get_current_dir(void) { return current_directory; }

int fs_change_directory(const char *name) {
  // Handle special cases
  if (strcmp(name, "/") == 0) {
    k_strncpy(current_directory, "/", FS_FILENAME_LEN);
    return 0;
  }

  if (strcmp(name, ".") == 0) {
    return 0; // Stay in current directory
  }

  if (strcmp(name, "..") == 0) {
    // Go back to root (simple implementation)
    k_strncpy(current_directory, "/", FS_FILENAME_LEN);
    return 0;
  }

  // Check if directory exists
  fs_file_entry_t *e = find_entry(name);
  if (!e) {
    return -1; // Directory not found
  }

  if (!e->is_directory) {
    return -2; // Not a directory
  }

  // Change to the directory
  k_strncpy(current_directory, name, FS_FILENAME_LEN);
  return 0;
}

char* fs_list_files_str(void) {
    size_t buf_size = 4096; // initial buffer size
    char *result = malloc(buf_size);
    if (!result) return NULL;
    result[0] = '\0';
    size_t used = 0;

    int prefix_len = 0;
    char prefix[FS_FILENAME_LEN];

    if (strcmp(current_directory, "/") != 0) {
        k_strncpy(prefix, current_directory, FS_FILENAME_LEN);
        prefix_len = 0;
        while (prefix[prefix_len]) prefix_len++;
        if (prefix_len > 0 && prefix[prefix_len - 1] != '/') {
            prefix[prefix_len++] = '/';
            prefix[prefix_len] = '\0';
        }
    }

    for (uint32_t i = 0; i < superblock.max_files; i++) {
        if (!file_table[i].used) continue;
        if (!is_valid_filename(file_table[i].name)) continue;

        char *display_name = file_table[i].name;

        if (prefix_len > 0) {
            if (k_strncmp(file_table[i].name, prefix, prefix_len) != 0) continue;
            display_name = file_table[i].name + prefix_len;
        } else {
            int has_slash = 0;
            for (size_t k = 0; file_table[i].name[k]; k++) {
                if (file_table[i].name[k] == '/') { has_slash = 1; break; }
            }
            if (has_slash) continue;
        }

        if (strcmp(display_name, "@") == 0 || strcmp(display_name, "2") == 0) continue;

        char line[128];
        size_t pos = 0;

        const char *tag = file_table[i].is_directory ? "[DIR] " : "      ";
        size_t j = 0;
        while (tag[j] && pos < sizeof(line) - 1) line[pos++] = tag[j++];

        j = 0;
        while (display_name[j] && pos < sizeof(line) - 1) line[pos++] = display_name[j++];

        if (!file_table[i].is_directory) {
            char dec[12];
            uint32_t s = file_table[i].size;
            int dpos = 0;
            if (s == 0) dec[dpos++] = '0';
            else {
                char rev[12];
                int r = 0;
                while (s) { rev[r++] = '0' + (s % 10); s /= 10; }
                while (r--) dec[dpos++] = rev[r];
            }
            dec[dpos] = '\0';

            const char *bytes = " bytes";
            if (pos < sizeof(line) - 1) line[pos++] = ' ';
            if (pos < sizeof(line) - 1) line[pos++] = '(';
            j = 0;
            while (dec[j] && pos < sizeof(line) - 1) line[pos++] = dec[j++];
            if (pos < sizeof(line) - 1) line[pos++] = ' ';
            j = 0;
            while (bytes[j] && pos < sizeof(line) - 1) line[pos++] = bytes[j++];
            if (pos < sizeof(line) - 1) line[pos++] = ')';
        }

        if (pos < sizeof(line) - 1) line[pos++] = '\n';
        line[pos] = '\0';

        size_t line_len = pos;
        if (used + line_len + 1 > buf_size) {
            // expand buffer
            size_t new_size = buf_size * 2;
            char *tmp = malloc(new_size);
            if (!tmp) { free(result); return NULL; }
            memcpy(tmp, result, used);
            free(result);
            result = tmp;
            buf_size = new_size;
        }

        memcpy(result + used, line, line_len);
        used += line_len;
        result[used] = '\0';
    }

    return result;
}
