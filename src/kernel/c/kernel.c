#include "boot/boot.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/interrupt.h"
#include "cpu/portio.h"
#include "driver/keyboard.h"
#include "driver/time.h"
#include "driver/tty.h"
#include "filesystem/fat32.h"
#include "kernel-entrypoint.h"
#include "memory/paging.h"
#include "process/process.h"
#include "process/scheduler.h"
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

	gdt_install_tss();
	set_tss_register();
	set_tss_kernel_current_stack();

	struct FAT32DriverRequest req;
	req.buf = 0;
	req.buffer_size = PAGE_FRAME_SIZE;
	req.parent_cluster_number = ROOT_CLUSTER_NUMBER;
	strcpy(req.ext, "", 3);

	strcpy(req.name, "ping", 8);
	process_create_user_process(&req);

	strcpy(req.name, "shell", 8);
	process_create_user_process(&req);

	/* Time setup, before starting timer */
	setup_time();
	scheduler_init();

	while (1) continue;
}
