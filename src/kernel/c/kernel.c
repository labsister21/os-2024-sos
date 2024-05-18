#include "boot/boot.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/interrupt.h"
#include "driver/time.h"
#include "filesystem/dev.h"
#include "filesystem/fat32.h"
#include "filesystem/proc.h"
#include "kernel-entrypoint.h"
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

	/* Mounting VFS */
	mount("/", &fat32_vfs);
	mount("/proc", &proc_vfs);
	mount("/dev", &dev_vfs);

	/* Framebuffer setup */
	framebuffer_clear();
	framebuffer_set_cursor(0, 0);

	gdt_install_tss();
	set_tss_register();
	set_tss_kernel_current_stack();

	process_create("/shell");
	process_create("/ping");
	// process_create("/ping");
	// process_create("/ping");
	// process_create("/ping");

	/* Time setup, before starting timer */
	setup_time();
	scheduler_init();

	while (1) continue;
}
