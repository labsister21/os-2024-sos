#include <stdint.h>
#include <stddef.h>
#include "header/stdlib/string.h"

void* memset(void *s, int c, size_t n) {
    uint8_t *buf = (uint8_t*) s;
    for (size_t i = 0; i < n; i++)
        buf[i] = (uint8_t) c;
    return s;
}

void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    for (size_t i = 0; i < n; i++)
        dstbuf[i] = srcbuf[i];
    return dstbuf;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *buf1 = (const uint8_t*) s1;
    const uint8_t *buf2 = (const uint8_t*) s2;
    for (size_t i = 0; i < n; i++) {
        if (buf1[i] < buf2[i])
            return -1;
        else if (buf1[i] > buf2[i])
            return 1;
    }

    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    if (dstbuf < srcbuf) {
        for (size_t i = 0; i < n; i++)
            dstbuf[i]   = srcbuf[i];
    } else {
        for (size_t i = n; i != 0; i--)
            dstbuf[i-1] = srcbuf[i-1];
    }

    return dest;
}

int strcmp(const char *str1, const char *str2, int size)
{
    int i;
    for (i = 0; i < size && str1[i] == str2[i]; i++)
    {
        if (str1[i] == '\0')
        {
            return 0;
        }
    }
    if (i == size)
    {
        return 0;
    }
    else
    {
        return str1[i] - str2[i];
    }
}

void strcpy(char *dest, const char *src, int max)
{
    while (*src != '\0' && max--) // Change max-- to --max
    {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
}

int strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len;
}

void append(char *dest, const char *src) {
    int idx = strlen(dest);
    for (int i = 0; i < strlen(src); i++)
    {
        dest[idx] = src[i];
        idx++;
    }
    dest[idx] = '\0';
}

void empty_string(char *str, int len) {
    for (int i = 0; i < len; i++)
    {
        str[i] = '\0';
    }
}