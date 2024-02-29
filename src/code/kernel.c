#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/driver/disk.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/fat32.h"
#include "header/kernel-entrypoint.h"
#include "header/stdlib/string.h"
#include "header/text/buffercolor.h"
#include "header/text/framebuffer.h"
#include <stdbool.h>
#include <stdint.h>

void kernel_setup(void) {
	load_gdt(&_gdt_gdtr);
	pic_remap();
	activate_keyboard_interrupt();
	keyboard_state_activate();
	initialize_idt();
	initialize_filesystem_fat32();
	framebuffer_clear();

	struct FAT32DriverRequest req;
	req.buf = "Afif";
	req.buffer_size = strlen(req.buf);
	strcpy(req.name, "test", 8);
	strcpy(req.ext, "x", 3);
	req.parent_cluster_number = ROOT_CLUSTER_NUMBER;

	write(&req);

	strcpy(req.name, "dir", 8);
	req.buffer_size = 0;
	write(&req);

	// struct FAT32DirectoryTable dir_table;
	// req.buf = &dir_table;
	// req.buffer_size = CLUSTER_SIZE;
	// read_directory(&req);
	//
	// req.parent_cluster_number = get_cluster_from_dir_entry(&dir_table.table[0]);
	// framebuffer_write(0, 1, req.parent_cluster_number + '0', WHITE, BLACK);
	// req.buf = "Afif";
	// req.buffer_size = strlen(req.buf);
	// strcpy(req.name, "test", 8);
	// strcpy(req.ext, "x", 3);
	// int res = write(&req);
	// framebuffer_write(0, 0, res + '0', WHITE, BLACK);

	// uint32_t size = CLUSTER_SIZE * 10;
	// uint8_t big[size];
	// for (uint32_t i = 0; i < size; ++i) big[i] = i % 10;
	//
	// strcpy(req.name, "Test2", 8);
	// req.buf = big;
	// req.buffer_size = size;
	// write(&req);
	//
	// uint8_t big_read[size];
	// req.buf = big_read;
	// req.buffer_size = size;
	// int ret = read(&req);
	// bool match = true;
	// uint8_t found, expect;
	// for (uint32_t i = 0; i < size; ++i) {
	// 	found = big_read[i];
	// 	expect = big[i];
	// 	if (found != expect) {
	// 		match = false;
	// 		break;
	// 	}
	// }
	//
	// framebuffer_clear();
	// framebuffer_write(0, 0, ret + '0', WHITE, BLACK);
	// framebuffer_write(1, 0, match + '0', WHITE, BLACK);
	// framebuffer_write(2, 0, found + '0', WHITE, BLACK);
	// framebuffer_write(3, 0, expect + '0', WHITE, BLACK);

	// struct ClusterBuffer dir_cluster;

	strcpy(req.name, "test", 8);
	strcpy(req.ext, "x", 3);
	int res = delete (&req);
	framebuffer_write(0, 0, '0' + res, WHITE, BLACK);

	strcpy(req.name, "dir2", 8);
	req.buffer_size = 0;
	write(&req);

	while (1) continue;
}
