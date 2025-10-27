#ifndef CLIB_H
#define CLIB_H

#include <stddef.h>
#include <stdint.h>

void outb(unsigned short port, unsigned char val);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
unsigned char inb(unsigned short port);
int strncmp(const char *s1, const char *s2, unsigned int n);
char *strncpy(char *dest, const char *src, unsigned int n);

#endif
