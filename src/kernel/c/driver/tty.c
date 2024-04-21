#include "driver/tty.h"
#include "driver/keyboard.h"
#include "text/framebuffer.h"

struct TTYState tty_state = {
		.size = 0,
		.current = 0
};

char fgetc() {
	if (tty_state.size == 0) {
		keyboard_state_activate();

		__asm__ volatile("sti"); // Allow hardware interrupt
		while (true) {
			__asm__ volatile("hlt");
			if (keyboard_state.buffer_filled) {
				char c;
				get_keyboard_buffer(&c);

				if (c == '\n') {
					framebuffer_next_line();
					tty_state.buffer[tty_state.size++] = c;
					break;
				} else if (c == '\b') {
					if (tty_state.size == 0) continue;
					--tty_state.size;

					framebuffer_set_cursor(framebuffer_state.row, framebuffer_state.col - 1);
					framebuffer_put(' ');
					framebuffer_set_cursor(framebuffer_state.row, framebuffer_state.col - 1);
					continue;
				}

				framebuffer_put(c);
				tty_state.buffer[tty_state.size++] = c;
			}
		}

		keyboard_state_deactivate();
	}
	char c = tty_state.buffer[tty_state.current++];
	if (tty_state.size == tty_state.current) {
		tty_state.size = 0;
		tty_state.current = 0;
	}
	return c;
}
