#include <syscall.h>
#include <time.h>
int main() {
	struct TimeRTC t;
	int stdout = syscall_VFS_OPEN("/dev/stdout_layered");
	// int x = 0;
	char time[24];
	while (1) {
		syscall_GET_TIME(&t);
		char fh = t.hour + 7;
		char fm = t.minute;
		char fs = t.second;
		int r = 24;
		int c = 72;
		for (int i = 0; i < 8; i++) {
			time[3 * i] = r;
			time[3 * i + 1] = c + i;
		}
		time[2] = '0' + fh / 10;
		time[5] = '0' + fh % 10;
		time[11] = '0' + fm / 10;
		time[14] = '0' + fm % 10;
		time[20] = '0' + fs / 10;
		time[23] = '0' + fs % 10;
		time[8] = ':';
		time[17] = ':';
		syscall_VFS_WRITE(stdout, time, 25);
		// x++;
	}

	return 0;
}
