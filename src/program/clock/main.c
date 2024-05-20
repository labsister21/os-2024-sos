#include <syscall.h>
int main() {

	int stdout = syscall_VFS_OPEN("/dev/stdout_layered");
	while (1) {
		syscall_VFS_WRITE(stdout, "\0\0c", 3);
	}

	return 0;
}
