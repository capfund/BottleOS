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
