#include <std/stdbool.h>
#include <std/stddef.h>
#include <std/stdint.h>
#include <std/string.h>
#include <syscall.h>

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

int str_len(char *str) {
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

/*
 * This function will destroy original string
 *
 */
char *strtok(char *str, char delimiter) {
	static char *current_str;
	static int i;
	if (str != NULL) {
		i = 0;
		current_str = str;
	}

	while (current_str[i] != '\0' && current_str[i] == delimiter) ++i;

	int start = i;
	if (current_str[i] == '\0') return NULL;

	while (current_str[i] != '\0' && current_str[i] != delimiter)
		++i;
	current_str[i] = '\0';
	++i;
	return &current_str[start];
};
