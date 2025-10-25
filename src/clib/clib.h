#ifndef CLIB_H
#define CLIB_H

#include <stddef.h>
#include <stdint.h>

int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
unsigned char inb(unsigned short port);

#endif
