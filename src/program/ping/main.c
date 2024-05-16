#include <syscall.h>
int main() {
	syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("Start");

	int flip = 0;
	while (1) {
		syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(flip ? "ping" : "pong");
		syscall_PUT_CHAR('\n');

		for (int i = 0; i < 100000000; ++i)
			;
		flip = !flip;
	}
	return 0;
}
