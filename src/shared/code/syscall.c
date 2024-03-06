#include <syscall.h>

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
	__asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
	__asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
	__asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
	__asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
	__asm__ volatile("int $0x30");
}
