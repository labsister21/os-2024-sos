#include "driver/tty.h"
#include "driver/keyboard.h"
#include "text/framebuffer.h"

struct TTYState tty_state = {
		.size = 0,
		.current_read = 0,
		.ansi_escape_size = 0,
};

bool check_ansi_escape_end(char c) {
	switch (c) {
	case '[':
	case ']':
	case '\\':
	case '^':
	case '_':
	case '`':
	case '{':
	case '|':
	case '}':
	case '~':
		return true;
	default: {
		if (
				('A' <= c && c <= 'Z') ||
				('a' <= c && c <= 'z')
		) {
			return true;
		}
		return false;
	}
	}
}

void process_ansi_escape() {
	char *buffer = tty_state.ansi_escape;
	char escape_end = buffer[tty_state.ansi_escape_size - 1];
	switch (escape_end) {
	case 'A':
		framebuffer_move_cursor(UP, 1);
		break;
	case 'B':
		framebuffer_move_cursor(DOWN, 1);
		break;
	case 'C':
		framebuffer_move_cursor(RIGHT, 1);
		break;
		framebuffer_move_cursor(LEFT, 1);
		break;
	case 'J':
		framebuffer_clear();
		break;
	}
}

void fputc(char c) {
	if (c == '\e' || tty_state.ansi_escape_size > 0) {
		tty_state.ansi_escape[tty_state.ansi_escape_size++] = c;
		if (check_ansi_escape_end(c)) {
			process_ansi_escape();
			tty_state.ansi_escape_size = 0;
		}
		return;
	}

	if (c == '\n') {
		framebuffer_next_line();
		return;
	}
	framebuffer_put(c);
	return;
}

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

					framebuffer_move_cursor(LEFT, 1);
					framebuffer_put(' ');
					framebuffer_move_cursor(LEFT, 1);
					continue;
				}

				if (' ' <= c && c <= '~') {
					framebuffer_put(c);
					tty_state.buffer[tty_state.size++] = c;
				}
			}
		}

		keyboard_state_deactivate();
	}
	char c = tty_state.buffer[tty_state.current_read++];
	if (tty_state.size == tty_state.current_read) {
		tty_state.size = 0;
		tty_state.current_read = 0;
	}
	return c;
}
