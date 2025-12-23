#include "clib.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return (unsigned char)(*s1) - (unsigned char)(*s2);
}

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

int strncmp(const char *s1, const char *s2, unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}

char *strncpy(char *dest, const char *src, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *ptr = dest;

    // Move to the end of dest
    while (*ptr != '\0') {
        ptr++;
    }

    // Copy at most n characters from src
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        ptr[i] = src[i];
    }

    ptr[i] = '\0'; // Null-terminate the result
    return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void *memset(void *dest, int value, size_t n) {
    uint8_t *d = (uint8_t*)dest;
    for (size_t i = 0; i < n; i++) {
        d[i] = (uint8_t)value;
    }
    return dest;
}

// simple free-list allocator
#define HEAP_SIZE (16 * 1024 * 1024) // 16MB heap

static uint8_t heap[HEAP_SIZE];

typedef struct block_header {
    size_t size;
    struct block_header *next;
    int free;
} block_header;

static block_header *free_list = NULL;

// simple owner accounting
#define MAX_OWNERS 32
typedef struct { const char *name; size_t bytes; unsigned long long cpu_ticks; } owner_entry;
static owner_entry owners[MAX_OWNERS];

static owner_entry *find_owner(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < MAX_OWNERS; ++i) {
        if (owners[i].name && strcmp(owners[i].name, name) == 0) return &owners[i];
    }
    return NULL;
}

void clib_account_alloc(const char *owner, size_t bytes) {
    if (!owner || bytes == 0) return;
    owner_entry *e = find_owner(owner);
    if (!e) {
        // find free slot
        for (int i = 0; i < MAX_OWNERS; ++i) {
            if (!owners[i].name) {
                owners[i].name = owner;
                owners[i].bytes = 0;
                e = &owners[i];
                break;
            }
        }
    }
    if (e) e->bytes += bytes;
}

void clib_account_free(const char *owner, size_t bytes) {
    if (!owner || bytes == 0) return;
    owner_entry *e = find_owner(owner);
    if (e) {
        if (e->bytes > bytes) e->bytes -= bytes;
        else e->bytes = 0;
    }
}

void clib_account_cpu(const char *owner, unsigned long long ticks) {
    if (!owner || ticks == 0) return;
    owner_entry *e = find_owner(owner);
    if (!e) {
        for (int i = 0; i < MAX_OWNERS; ++i) {
            if (!owners[i].name) {
                owners[i].name = owner;
                owners[i].bytes = 0;
                owners[i].cpu_ticks = 0;
                e = &owners[i];
                break;
            }
        }
    }
    if (e) e->cpu_ticks += ticks;
}

unsigned long long clib_owner_cpu_ticks(int idx) {
    int c = 0;
    for (int i = 0; i < MAX_OWNERS; ++i) {
        if (!owners[i].name) continue;
        if (c == idx) return owners[i].cpu_ticks;
        c++;
    }
    return 0ULL;
}

unsigned long long clib_total_cpu_ticks(void) {
    unsigned long long total = 0ULL;
    for (int i = 0; i < MAX_OWNERS; ++i) {
        if (!owners[i].name) continue;
        total += owners[i].cpu_ticks;
    }
    return total;
}

void clib_cpu_frame_decay(void) {
    // decay all owner cpu counters (simple right shift by 1)
    for (int i = 0; i < MAX_OWNERS; ++i) {
        if (!owners[i].name) continue;
        owners[i].cpu_ticks >>= 1;
    }
}

int clib_owner_count(void) {
    int c = 0;
    for (int i = 0; i < MAX_OWNERS; ++i) if (owners[i].name) c++;
    return c;
}

const char *clib_owner_name(int idx) {
    int c = 0;
    for (int i = 0; i < MAX_OWNERS; ++i) {
        if (!owners[i].name) continue;
        if (c == idx) return owners[i].name;
        c++;
    }
    return NULL;
}

size_t clib_owner_bytes(int idx) {
    int c = 0;
    for (int i = 0; i < MAX_OWNERS; ++i) {
        if (!owners[i].name) continue;
        if (c == idx) return owners[i].bytes;
        c++;
    }
    return 0;
}

// forward declare panic from vesa driver (display on framebuffer)
extern void vesa_kernel_panic(const char *msg);

static size_t align8(size_t v) {
    return (v + 7) & ~((size_t)7);
}

void *malloc(size_t size) {
    if (size == 0) return NULL;
    size = align8(size);

    if (!free_list) {
        // initialize a single free block covering the whole heap
        free_list = (block_header *)heap;
        free_list->size = HEAP_SIZE - sizeof(block_header);
        free_list->next = NULL;
        free_list->free = 1;
    }

    block_header *cur = free_list;

    while (cur) {
        if (cur->free && cur->size >= size) {
            // found a fit
            if (cur->size >= size + sizeof(block_header) + 8) {
                // split
                uint8_t *split_at = (uint8_t *)cur + sizeof(block_header) + size;
                block_header *newb = (block_header *)split_at;
                newb->size = cur->size - size - sizeof(block_header);
                newb->next = cur->next;
                newb->free = 1;

                cur->size = size;
                cur->next = newb;
            }

            cur->free = 0;
            return (void *)((uint8_t *)cur + sizeof(block_header));
        }
        cur = cur->next;
    }

    // out of memory: trigger kernel panic (if available)
    vesa_kernel_panic("malloc(): out of memory\n");
    return NULL;
}

void free(void *ptr) {
    if (!ptr) return;
    block_header *hdr = (block_header *)((uint8_t *)ptr - sizeof(block_header));
    if (hdr->free)
        vesa_kernel_panic("free(): double free"); // double free detected
    hdr->free = 1;

    // coalesce adjacent free blocks
    block_header *cur = free_list;
    while (cur && cur->next) {
        uint8_t *cur_end = (uint8_t *)cur + sizeof(block_header) + cur->size;
        if (cur->free && cur->next->free && cur_end == (uint8_t *)cur->next) {
            cur->size += sizeof(block_header) + cur->next->size;
            cur->next = cur->next->next;
            continue; // try coalescing again at same cur
        }
        cur = cur->next;
    }
}

void malloc_reset(void) {
    free_list = NULL;
}

void clib_heap_stats(size_t *total, size_t *used, size_t *free_out) {
    if (total) *total = HEAP_SIZE;
    if (!free_list) {
        if (used) *used = 0;
        if (free_out) *free_out = HEAP_SIZE;
        return;
    }

    // compute used by scanning blocks
    size_t used_bytes = 0;
    block_header *cur = free_list;
    // The heap begins at `heap` and contains linked blocks that may be free or used.
    while (cur) {
        used_bytes += (cur->free ? 0 : cur->size) + sizeof(block_header);
        cur = cur->next;
    }

    if (used) *used = used_bytes;
    if (free_out) *free_out = HEAP_SIZE - used_bytes;
}

void clib_heap_inspect(
    size_t *total,
    size_t *used,
    size_t *free_out,
    size_t *num_blocks,
    size_t *num_free_blocks,
    size_t *largest_free,
    size_t *largest_used
) {
    if (total) *total = HEAP_SIZE;
    if (!free_list) {
        if (used) *used = 0;
        if (free_out) *free_out = HEAP_SIZE;
        if (num_blocks) *num_blocks = 0;
        if (num_free_blocks) *num_free_blocks = 1;
        if (largest_free) *largest_free = HEAP_SIZE - sizeof(block_header);
        if (largest_used) *largest_used = 0;
        return;
    }

    size_t used_bytes = 0;
    size_t blocks = 0;
    size_t free_blocks = 0;
    size_t max_free = 0;
    size_t max_used = 0;

    block_header *cur = free_list;
    while (cur) {
        blocks++;
        if (cur->free) {
            free_blocks++;
            if (cur->size > max_free) max_free = cur->size;
        } else {
            if (cur->size > max_used) max_used = cur->size;
            used_bytes += cur->size + sizeof(block_header);
        }
        cur = cur->next;
    }

    if (used) *used = used_bytes;
    if (free_out) *free_out = HEAP_SIZE - used_bytes;
    if (num_blocks) *num_blocks = blocks;
    if (num_free_blocks) *num_free_blocks = free_blocks;
    if (largest_free) *largest_free = max_free;
    if (largest_used) *largest_used = max_used;
}

//
//
// other stdlibs
//
//

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++)) {
        // copy until null terminator
    }
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;            // find end of dest
    while (*src) *d++ = *src++; // copy src
    *d = '\0';
    return dest;
}

int atoi(const char *s) {
    if (!s) return 0;
    int sign = 1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '+') { s++; }
    else if (*s == '-') { sign = -1; s++; }
    int val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val * sign;
}

// Provide libgcc-style 64-bit unsigned division helper to avoid linker
// dependency on compiler-rt/libgcc. Simple long division algorithm.
unsigned long long __udivdi3(unsigned long long n, unsigned long long d) {
    if (d == 0) {
        // old beh: return (unsigned long long)-1;
        vesa_kernel_panic("__udivdi3: division by zero\n");
    }
    unsigned long long q = 0;
    unsigned long long r = 0;
    for (int i = 63; i >= 0; --i) {
        r = (r << 1) | ((n >> i) & 1ULL);
        if (r >= d) {
            r -= d;
            q |= (1ULL << i);
        }
    }
    return q;
}

void utoa_dec(unsigned int value, char *buf) {
    char tmp[16];
    int i = 0, j = 0;

    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (value > 0) {
        tmp[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        buf[j++] = tmp[--i];
    }

    buf[j] = '\0';
}

void str_append(char *dst, const char *src, int dst_max)
{
    int i = 0;
    int j = 0;

    // find end of dst
    while (i < dst_max && dst[i] != '\0')
        i++;

    // append src
    while (i < dst_max - 1 && src[j] != '\0') {
        dst[i++] = src[j++];
    }

    dst[i] = '\0';
}