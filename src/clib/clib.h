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
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);

#endif
