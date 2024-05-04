#include <syscall.h>
int main() {
	syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("ping");
	return 0;
}
