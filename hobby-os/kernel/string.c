#include "string.h"

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dst, const char *src) {
    char *out = dst;
    while ((*dst++ = *src++)) {
    }
    return out;
}

void *memset(void *dst, int value, size_t n) {
    unsigned char *p = dst;
    while (n--) {
        *p++ = (unsigned char)value;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void utoa(uint32_t value, char *buf, int base) {
    char tmp[32];
    int i = 0;
    if (base < 2 || base > 16) {
        buf[0] = '\0';
        return;
    }
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (value) {
        uint32_t digit = value % (uint32_t)base;
        tmp[i++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
        value /= (uint32_t)base;
    }
    int j = 0;
    while (i--) {
        buf[j++] = tmp[i];
    }
    buf[j] = '\0';
}

void itoa(int32_t value, char *buf, int base) {
    if (value < 0 && base == 10) {
        *buf++ = '-';
        utoa((uint32_t)(-(int64_t)value), buf, base);
    } else {
        utoa((uint32_t)value, buf, base);
    }
}
