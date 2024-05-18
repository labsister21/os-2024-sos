#include <std/stdbool.h>
#include <syscall.h>
#include <vfs.h>
int main() {
	syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("ping");
	syscall_PUT_CHAR('\n');
	struct VFSEntry entry;
	syscall_VFS_STAT("/proc", &entry);
	syscall_EXEC("/ping");

	// bool flip = true;
	// for (int i = 0; i < 10; ++i) {
	// 	for (int i = 0; i < 100000000; ++i)
	// 		;
	// 	syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(flip ? "ping" : "pong");
	// 	syscall_PUT_CHAR('\n');
	// 	flip = !flip;
	// }
	return 0;
}
