#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <std/stdint.h>

static inline void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
	__asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
	__asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
	__asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
	__asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
	__asm__ volatile("int $0x30");
}

#define SYSCALL(syscall_number, ...)                         \
	static inline void syscall_##syscall_number(__VA_ARGS__) { \
		syscall(__VA_ARGS__)                                     \
	}

#define SYSCALL_0(syscall_number)                 \
	static inline void syscall_##syscall_number() { \
		syscall(syscall_number, 0, 0, 0);             \
	}

#define SYSCALL_1(syscall_number, first_type, first_param)              \
	static inline void syscall_##syscall_number(first_type first_param) { \
		syscall(syscall_number, (uint32_t)first_param, 0, 0);               \
	}

#define SYSCALL_2(syscall_number, first_type, first_param, second_type, second_param)             \
	static inline void syscall_##syscall_number(first_type first_param, second_type second_param) { \
		syscall(syscall_number, (uint32_t)first_param, (uint32_t)second_param, 0);                    \
	}

#define SYSCALL_3(syscall_number, first_type, first_param, second_type, second_param, third_type, third_param)            \
	static inline void syscall_##syscall_number(first_type first_param, second_type second_param, third_type third_param) { \
		syscall(syscall_number, (uint32_t)first_param, (uint32_t)second_param, 0);                                            \
	}

// Disk
#include <fat32.h>
#define READ 0
SYSCALL_2(READ, struct FAT32DriverRequest *, req, int8_t *, ret);

#define READ_DIRECTORY 1
SYSCALL_2(READ_DIRECTORY, struct FAT32DriverRequest *, req, int8_t *, ret);

#define WRITE 2
SYSCALL_2(WRITE, struct FAT32DriverRequest *, req, int8_t *, ret);

#define DELETE 3
SYSCALL_2(DELETE, struct FAT32DriverRequest *, req, int8_t *, ret);

// Input
#define GET_CHAR 4
SYSCALL_1(GET_CHAR, char *, c);

#define GET_CHAR_NON_BLOCKING 40
SYSCALL_1(GET_CHAR_NON_BLOCKING, char *, c);

#define PUT_CHAR 5
SYSCALL_1(PUT_CHAR, char, c);

// Framebuffer
#define FRAMEBUFFER_PUT_CHAR 6
SYSCALL_1(FRAMEBUFFER_PUT_CHAR, char, c);

#define FRAMEBUFFER_PUT_CHARS 7
SYSCALL_2(FRAMEBUFFER_PUT_CHARS, char *, c, int, size);

#define FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS 8
SYSCALL_1(FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS, char *, c);

#define FRAMEBUFFER_CLEAR 9
SYSCALL_0(FRAMEBUFFER_CLEAR);

#define FRAMEBUFFER_CURSOR 10
SYSCALL_2(FRAMEBUFFER_CURSOR, int, x, int, y);

#endif
