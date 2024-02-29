#include "header/stdlib/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void *memset(void *s, int c, size_t n) {
	uint8_t *buf = (uint8_t *)s;
	for (size_t i = 0; i < n; i++) buf[i] = (uint8_t)c;
	return s;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
	uint8_t *dstbuf = (uint8_t *)dest;
	const uint8_t *srcbuf = (const uint8_t *)src;
	for (size_t i = 0; i < n; i++) dstbuf[i] = srcbuf[i];
	return dstbuf;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	const uint8_t *buf1 = (const uint8_t *)s1;
	const uint8_t *buf2 = (const uint8_t *)s2;
	for (size_t i = 0; i < n; i++) {
		if (buf1[i] < buf2[i]) return -1;
		else if (buf1[i] > buf2[i]) return 1;
	}

	return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
	uint8_t *dstbuf = (uint8_t *)dest;
	const uint8_t *srcbuf = (const uint8_t *)src;
	if (dstbuf < srcbuf) {
		for (size_t i = 0; i < n; i++) dstbuf[i] = srcbuf[i];
	} else {
		for (size_t i = n; i != 0; i--) dstbuf[i - 1] = srcbuf[i - 1];
	}

	return dest;
}

int strcmp(char *str1, char *str2) {
	int i = 0;
	while (true) {
		if (str1[i] < str2[i]) return -1;
		else if (str1[i] > str2[i]) return 1;
		else if (str1[i] == '\0' && str2[i] == '\0') break;
		else if (str1[i] == '\0' && str2[i] != '\0') return -1;
		else if (str1[i] != '\0' && str2[i] == '\0') return 1;
		++i;
	}
	return 0;
}

int strlen(char *str) {
	int i = 0;
	while (str[i] != '\0') ++i;
	return i;
}

void strcpy(char *dst, char *src, int size) {
	int i = 0;
	while (i < size - 1 && src[i] != '\0') {
		dst[i] = src[i];
		++i;
	}
	dst[i] = '\0';
};
