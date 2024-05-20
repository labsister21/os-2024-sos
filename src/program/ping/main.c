#include <std/stdbool.h>
#include <std/string.h>
#include <syscall.h>
#include <vfs.h>
int main() {
	// syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("ping");
	// syscall_PUT_CHAR('\n');
	// struct VFSEntry entry;
	// syscall_VFS_STAT("/proc", &entry);
	// syscall_EXEC("/ping");

	syscall_SLEEP(3);
	int stdout = syscall_VFS_OPEN("/dev/stdout_layered");
	while (true) {
		syscall_VFS_WRITE(stdout, "\0\0p", 3);
	}

	// char buff[10];
	// while (true) {
	// 	int count = syscall_VFS_READ(stdout, buff, 10);
	// 	if (count > 0) {
	// 		for (int i = 0; i < count; ++i) {
	// 			syscall_PUT_CHAR(buff[i]);
	// 		}
	// 	}
	// }

	// char *text = "TULISAN";
	// syscall_VFS_WRITE(stdout, text, str_len(text) + 1);

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
