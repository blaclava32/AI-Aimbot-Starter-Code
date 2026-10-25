#include "string.h"

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && (*a == *b)) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dst, const char *src) {
    char *out = dst;
    while ((*dst++ = *src++)) {}
    return out;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}

char *strcat(char *dst, const char *src) {
    char *out = dst;
    while (*dst) dst++;
    while ((*dst++ = *src++)) {}
    return out;
}

char *strncat(char *dst, const char *src, size_t n) {
    char *out = dst;
    while (*dst) dst++;
    while (n && *src) {
        *dst++ = *src++;
        n--;
    }
    *dst = '\0';
    return out;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return c == 0 ? (char *)s : NULL;
}

void *memset(void *dst, int value, size_t n) {
    unsigned char *p = dst;
    while (n--) *p++ = (unsigned char)value;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = a, *y = b;
    while (n--) {
        if (*x != *y) return *x - *y;
        x++; y++;
    }
    return 0;
}

void utoa(uint32_t value, char *buf, int base) {
    char tmp[33];
    int i = 0;
    if (base < 2 || base > 16) { buf[0] = '\0'; return; }
    if (value == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (value) {
        uint32_t digit = value % (uint32_t)base;
        tmp[i++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
        value /= (uint32_t)base;
    }
    int j = 0;
    while (i--) buf[j++] = tmp[i];
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

int atoi(const char *s) {
    int sign = 1, v = 0;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return sign * v;
}

uint32_t parse_hex(const char *s) {
    uint32_t v = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (*s) {
        char c = *s++;
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (uint32_t)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (uint32_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (uint32_t)(c - 'A' + 10);
        else break;
    }
    return v;
}
