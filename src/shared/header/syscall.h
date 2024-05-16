#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <std/stdint.h>
#include <time.h>
#include <vfs.h>

static inline int syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
	__asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
	__asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
	__asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
	__asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
	__asm__ volatile("int $0x30");

	uint32_t result = 0;
	__asm__ volatile("mov %%eax,%0" : "=r"(result));
	return result;
}

#define SYSCALL(syscall_number, ...)                        \
	static inline int syscall_##syscall_number(__VA_ARGS__) { \
		return syscall(__VA_ARGS__)                             \
	}

#define SYSCALL_0(syscall_number)                \
	static inline int syscall_##syscall_number() { \
		return syscall(syscall_number, 0, 0, 0);     \
	}

#define SYSCALL_1(syscall_number, first_type, first_param)             \
	static inline int syscall_##syscall_number(first_type first_param) { \
		return syscall(syscall_number, (uint32_t)first_param, 0, 0);       \
	}

#define SYSCALL_2(syscall_number, first_type, first_param, second_type, second_param)            \
	static inline int syscall_##syscall_number(first_type first_param, second_type second_param) { \
		return syscall(syscall_number, (uint32_t)first_param, (uint32_t)second_param, 0);            \
	}

#define SYSCALL_3(syscall_number, first_type, first_param, second_type, second_param, third_type, third_param)           \
	static inline int syscall_##syscall_number(first_type first_param, second_type second_param, third_type third_param) { \
		return syscall(syscall_number, (uint32_t)first_param, (uint32_t)second_param, (uint32_t)third_param);                \
	}

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

// Time
#define GET_TIME 11
SYSCALL_1(GET_TIME, struct TimeRTC *, t);

// Process
#define EXEC 121
SYSCALL_1(EXEC, char *, path);

#define KILL 122
SYSCALL_1(KILL, int, pid);

// VFS
#define VFS_STAT 131
SYSCALL_2(VFS_STAT, char *, path, struct VFSEntry *, entry)

#define VFS_DIR_STAT 132
SYSCALL_2(VFS_DIR_STAT, char *, path, struct VFSEntry *, entries)

#define VFS_OPEN 133
SYSCALL_1(VFS_OPEN, char *, path)

#define VFS_CLOSE 134
SYSCALL_1(VFS_CLOSE, int, fd)

#define VFS_READ 135
SYSCALL_3(VFS_READ, int, fd, char *, buffer, int, size)

#define VFS_WRITE 136
SYSCALL_3(VFS_WRITE, int, fd, char *, buffer, int, size)

#define VFS_MKFILE 137
SYSCALL_1(VFS_MKFILE, char *, path)

#define VFS_MKDIR 138
SYSCALL_1(VFS_MKDIR, char *, path)

#define VFS_DELETE 139
SYSCALL_1(VFS_DELETE, char *, path)

#endif
