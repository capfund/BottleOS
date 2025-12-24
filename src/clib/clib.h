#ifndef CLIB_H
#define CLIB_H

#include <stddef.h>
#include <stdint.h>

// inlines
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}

// remainder of defs

void outb(unsigned short port, unsigned char val);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
unsigned char inb(unsigned short port);
int strncmp(const char *s1, const char *s2, unsigned int n);
char *strncpy(char *dest, const char *src, unsigned int n);
void *memset(void *dest, int value, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
char* strncat(char *dest, const char *src, size_t n);
void *malloc(size_t size);
void free(void *ptr);
void malloc_reset(void);
// Heap statistics: total heap bytes, used bytes, free bytes
void clib_heap_stats(size_t *total, size_t *used, size_t *free_out);
// Extended heap inspection: number of blocks, number of free blocks,
// largest free block size, largest used block size
void clib_heap_inspect(
    size_t *total,
    size_t *used,
    size_t *free_out,
    size_t *num_blocks,
    size_t *num_free_blocks,
    size_t *largest_free,
    size_t *largest_used
);

// Simple owner accounting for attributing allocations to logical owners
void clib_account_alloc(const char *owner, size_t bytes);
void clib_account_free(const char *owner, size_t bytes);
int  clib_owner_count(void);
const char *clib_owner_name(int idx);
size_t clib_owner_bytes(int idx);
// CPU accounting
void clib_account_cpu(const char *owner, unsigned long long ticks);
unsigned long long clib_owner_cpu_ticks(int idx);
unsigned long long clib_total_cpu_ticks(void);
// called each frame to decay cpu counters (keeps recent activity weighted)
void clib_cpu_frame_decay(void);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
int atoi(const char *s);
void utoa_dec(unsigned int value, char *buf);
void str_append(char *dst, const char *src, int dst_max);
char* strtok(char *str, const char *delim);

#endif
