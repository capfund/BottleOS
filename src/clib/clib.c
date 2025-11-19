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

// allocs clib
#define HEAP_SIZE (1024 * 256) // 64 KB heap (adjust as needed)

static uint8_t heap[HEAP_SIZE];
static size_t heap_index = 0;

void *malloc(size_t size) {
    if (heap_index + size >= HEAP_SIZE) {
        return 0; // out of memory
    }

    void *ptr = &heap[heap_index];
    heap_index += size;

    // Align heap_index to 8 bytes for safety
    if (heap_index & 7)
        heap_index = (heap_index + 7) & ~7;

    return ptr;
}

void free(void *ptr) {
    // No-op in this simple bump allocator
    // You can later implement block freeing if needed
    (void)ptr;
}

void malloc_reset(void) {
    heap_index = 0; // optional helper to reset heap (like after a program exits)
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
