#include "boot/boot.h"

static multiboot_info_t *mbi;

void init_multiboot_info() {
	asm("\t movl %%ebx,%0" : "=r"(mbi));
};

multiboot_info_t *get_multiboot_info() {
	return mbi;
};
