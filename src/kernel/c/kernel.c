#include "boot/boot.h"
#include "boot/multiboot.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/interrupt.h"
#include "filesystem/fat32.h"
#include "kernel-entrypoint.h"
#include "memory/paging.h"
#include "text/framebuffer.h"
#include <std/stdbool.h>
#include <std/stdint.h>
#include <std/string.h>

void kernel_setup(void) {
	/* Multiboot Info setup */
	init_multiboot_info();

	/* Global Descriptor Table setup*/
	load_gdt(&_gdt_gdtr);

	/* Interrrupt setup */
	pic_remap();
	initialize_idt();
	activate_keyboard_interrupt();

	/* Filesystem setup */
	initialize_filesystem_fat32();

	/* Framebuffer setup */
	framebuffer_clear();
	framebuffer_set_cursor(0, 0);

	// gdt_install_tss();
	// set_tss_register();
	//
	// void *mem = 0;
	// paging_allocate_user_page_frame(&_paging_kernel_page_directory, mem);
	//
	// struct FAT32DriverRequest req;
	// req.buf = mem;
	// req.buffer_size = PAGE_FRAME_SIZE;
	// req.parent_cluster_number = ROOT_CLUSTER_NUMBER;
	// strcpy(req.name, "shell", 8);
	// strcpy(req.ext, "", 3);
	// read(&req);
	//
	// set_tss_kernel_current_stack();
	// kernel_execute_user_program(mem);

	while (1) continue;
}
