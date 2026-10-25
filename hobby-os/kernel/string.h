#ifndef SPARK_STRING_H
#define SPARK_STRING_H

#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strcpy(char *dst, const char *src);
void *memset(void *dst, int value, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void itoa(int32_t value, char *buf, int base);
void utoa(uint32_t value, char *buf, int base);

#endif
