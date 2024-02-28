#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/driver/disk.h"
#include "header/driver/keyboard.h"
#include "header/kernel-entrypoint.h"
#include "header/text/buffercolor.h"
#include "header/text/framebuffer.h"
#include <stdbool.h>

void kernel_setup(void) {
	load_gdt(&_gdt_gdtr);
	pic_remap();
	activate_keyboard_interrupt();
	keyboard_state_activate();
	initialize_idt();

	struct BlockBuffer b;
	for (int i = 0; i < 512; i++)
		b.buf[i] = i % 16;
	write_blocks(&b, 17, 1);
	while (true)
		;

	framebuffer_clear();
	framebuffer_set_cursor(0, 0);
	int pos = 0;
	while (true) {
		char c;
		get_keyboard_buffer(&c);

		if (c == 0)
			continue;

		bool printable = true;
		if (c == '\n') {
			pos = ((pos / BUFFER_WIDTH) + 1) * BUFFER_WIDTH;
			printable = false;
		}

		if (c == '\b') {
			if (pos == 0)
				continue;
			pos = pos - 1;
			printable = false;
			framebuffer_write(
					pos / BUFFER_WIDTH, pos % BUFFER_WIDTH, ' ', WHITE, BLACK);
		}

		if (printable) {
			framebuffer_write(
					pos / BUFFER_WIDTH, pos % BUFFER_WIDTH, c, WHITE, BLACK);
			pos = pos + 1;
		}

		framebuffer_set_cursor(pos / BUFFER_WIDTH, pos % BUFFER_WIDTH);
	}
}
