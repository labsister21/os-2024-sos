#include "boot/boot.h"
#include "boot/multiboot.h"

static multiboot_info_t *mbi;

void init_multiboot_info() {
	uint32_t addr;
	asm("\t movl %%ebx,%0" : "=r"(addr));
	addr += 0xC0000000;
	mbi = (multiboot_info_t *)addr;
};

multiboot_info_t *get_multiboot_info() {
	return mbi;
};

uint32_t get_memory_size() {
	multiboot_info_t *mbi = get_multiboot_info();

	uint32_t size = 0;
	multiboot_memory_map_t *mma = (multiboot_memory_map_t *)(mbi->mmap_addr + 0xC0000000);
	for (uint32_t i = 0; i < mbi->mmap_length; ++i)
		if (mma[i].type == MULTIBOOT_MEMORY_AVAILABLE)
			size += mma[i].len;

	return size;
};
