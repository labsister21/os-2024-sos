#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <std/stdint.h>
void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

#define SYSCALL(syscall_number, ...) \
	void syscall_##syscall_number(__VA_ARGS__)

// Disk
#include <fat32.h>
#define READ 0
SYSCALL(READ, struct FAT32DriverRequest *);

#define READ_DIRECTORY 1
SYSCALL(READ_DIRECTORY, struct FAT32DriverRequest *);

#define WRITE 2
SYSCALL(WRITE, struct FAT32DriverRequest *);

#define DELETE 3
SYSCALL(DELETE, struct FAT32DriverRequest *);

// Input
#define GET_CHAR 4
SYSCALL(GET_CHAR, struct FAT32DriverRequest *);

// Framebuffer
#define FRAMEBUFFER_PUT_CHAR 5
SYSCALL(PUT_CHAR, struct FAT32DriverRequest *);

#define FRAMEBUFFER_PUT_CHARS 6
SYSCALL(PUT_CHARS, struct FAT32DriverRequest *);

#define FRAMEBUFFER_CLEAR 7
SYSCALL(FRAMEBUFFER_CLEAR, struct FAT32DriverRequest *);

#define FRAMEBUFFER_CURSOR 8
SYSCALL(FRAMEBUFFER_CURSOR, struct FAT32DriverRequest *);

#endif
