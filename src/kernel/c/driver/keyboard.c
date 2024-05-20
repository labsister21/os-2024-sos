#include "driver/keyboard.h"
#include "cpu/interrupt.h"
#include "cpu/portio.h"
#include "filesystem/dev.h"
#include "text/framebuffer.h"
#include <std/stdint.h>

// clang-format off
const char keyboard_scancode_1_to_ascii_map[256] = {
    0,   0x1B, '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,

    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,
};

const char keyboard_scancode_1_to_ascii_map_shifted[256] = {
    0,   0x1B, '!',  '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
    '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    ':', '"',  '~',  0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?',  0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,

    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,
};
// clang-format on

struct KeyboardDriverState keyboard_state = {
		.keyboard_input_on = false,
		.keyboard_buffer = '\0',
		.shift_modifier = false,
		.buffer_filled = false,
		.alt_modifier = false,
		.ctrl_modifier = false
};

void keyboard_state_activate() { keyboard_state.keyboard_input_on = true; }

void keyboard_state_deactivate() { keyboard_state.keyboard_input_on = false; }

void get_keyboard_buffer(char *buff) {
	*buff = keyboard_state.keyboard_buffer;
	keyboard_state.keyboard_buffer = '\0';
	keyboard_state.buffer_filled = false;
}

void keyboard_isr() {
	uint8_t scancode = in(KEYBOARD_DATA_PORT);
	pic_ack(PIC1_OFFSET + IRQ_KEYBOARD);

	// if (!keyboard_state.keyboard_input_on) return;
	switch (scancode) {

	case EXT_SCANCODE_RIGHT_SHIFT + MAKE_OFFSET:
	case EXT_SCANCODE_LEFT_SHIFT + MAKE_OFFSET:
		keyboard_state.shift_modifier = true;
		break;

	case EXT_SCANCODE_ALT + MAKE_OFFSET:
		keyboard_state.shift_modifier = true;
		break;

	case EXT_SCANCODE_CTRL + MAKE_OFFSET:
		keyboard_state.ctrl_modifier = true;
		break;

	case EXT_SCANCODE_RIGHT_SHIFT + BREAK_OFFSET:
	case EXT_SCANCODE_LEFT_SHIFT + BREAK_OFFSET:
		keyboard_state.shift_modifier = false;
		break;

	case EXT_SCANCODE_ALT + BREAK_OFFSET:
		keyboard_state.shift_modifier = false;
		break;

	case EXT_SCANCODE_CTRL + BREAK_OFFSET:
		keyboard_state.ctrl_modifier = false;
		break;

	default: {
		char c;
		if (keyboard_state.shift_modifier)
			c = keyboard_scancode_1_to_ascii_map_shifted[scancode];
		else c = keyboard_scancode_1_to_ascii_map[scancode];

		if (scancode & BREAK_OFFSET) return;
		keyboard_state.buffer_filled = true;
		keyboard_state.keyboard_buffer = c;
		fill_stdin_buffer(c);
	}
	}
}
