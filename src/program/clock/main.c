#include <syscall.h>
int main() {

	int stdout = syscall_VFS_OPEN("/dev/stdout_layered");
	while (1) {
		char payload[3];
		payload[0] = 24;
		payload[1] = 79;
		payload[2] = 'v';
		syscall_VFS_WRITE(stdout, payload, 3);
	}

	return 0;
}
