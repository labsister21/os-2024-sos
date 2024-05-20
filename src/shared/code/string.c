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

char *strtok(char *str, char delimiter) {
	static char *current_str;
	static int i;
	static bool finished = false;
	if (str != NULL) {
		i = 0;
		current_str = str;
		finished = false;
	}
	if (finished) return NULL;

	while (current_str[i] != '\0' && current_str[i] == delimiter) ++i;

	int start = i;
	if (current_str[i] == '\0') return NULL;

	while (current_str[i] != '\0' && current_str[i] != delimiter) ++i;

	if (current_str[i] == '\0') finished = true;
	current_str[i++] = '\0';

	return &current_str[start];
};

int strtoi(char *str, char **end) {
	int result = -1;
	while (*str != '\0') {
		if ('0' <= *str && *str <= '9') {
			if (result == -1) result = 0;

			result = 10 * result + (*str - '0');
			if (end != NULL)
				*end = str + 1;
		} else {
			if (result != -1) break;
		}

		str += 1;
	}
	return result;
};

void strcat(char *dst, char *src, int max) {
	int i = 0;
	while (i < max - 1 && dst[i] != '\0') ++i;

	int j = 0;
	while (i < max - 1 && src[j] != '\0') dst[i++] = src[j++];
	dst[i] = '\0';
}

static char to_digit(int digit) {
	if (0 <= digit && digit <= 9) return digit + '0';
	else if (10 <= digit && digit <= 15) return (digit - 10) + 'A';
	return ' ';
}

void strrev(char *str) {
	int size = str_len(str);
	for (int i = 0; i < (size / 2); ++i) {
		int left = i;
		int right = (size - 1) - left;

		char t = str[left];
		str[left] = str[right];
		str[right] = t;
	}
};

void itoa(int value, char *str, int base) {
	bool neg = value < 0;
	char *start = str;

	if (neg) value *= -1;

	do {
		*str = to_digit(value % base);
		value /= base;
		str += 1;
	} while (value > 0);
	if (neg) {
		*str = '-';
		str += 1;
	}
	*str = '\0';

	strrev(start);
}

int len(char arr[100][1024]) {
	int i = 0;
	while (*arr[i] != '\0') {
		i++;
	}
	return i;
}

void push(char arr[100][1024], char val[1024]) {
	strcpy(arr[len(arr)], val, 1024);
}

void pop(char arr[100][1024]) {
	strcpy(arr[len(arr)-1], "\0", 1024);
}