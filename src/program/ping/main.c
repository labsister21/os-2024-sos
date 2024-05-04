#include <syscall.h>
int main() {

	int flip = 0;
	while (1) {
		for (int i = 0; i < 100000000; ++i)
			;
		syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(flip ? "ping" : "pong");
		syscall_PUT_CHAR('\n');

		flip = !flip;
	}
	return 0;
}
