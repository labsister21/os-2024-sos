#include "util.h"
void puts(char *str) {
	syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(str);
}