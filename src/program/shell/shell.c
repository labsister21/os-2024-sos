#include <std/stdint.h>
#include <syscall.h>

int main(void) {
	char n = '_';
	while (1) {
		syscall(0x4, (uint32_t)&n, 0, 0);
		syscall(0x5, (uint32_t)&n, 0xF, 0);
	}
	return 0;
}
