#include "clib.h"

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
  __asm__("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}
