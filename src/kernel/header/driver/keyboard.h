#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <std/stdbool.h>

#define EXT_SCANCODE_UP 0x48
#define EXT_SCANCODE_DOWN 0x50
#define EXT_SCANCODE_LEFT 0x4B
#define EXT_SCANCODE_RIGHT 0x4D

#define EXT_SCANCODE_ALT 0x38
#define EXT_SCANCODE_RIGHT_SHIFT 0x2A
#define EXT_SCANCODE_LEFT_SHIFT 0x36
#define EXT_SCANCODE_CTRL 0x1D

#define KEYBOARD_DATA_PORT 0x60
#define EXTENDED_SCANCODE_BYTE 0xE0

#define MAKE_OFFSET 0x00
#define BREAK_OFFSET 0x80

/**
 * keyboard_scancode_1_to_ascii_map[256], Convert scancode values that
 * correspond to ASCII printables How to use this array: ascii_char =
 * k[scancode]
 *
 * By default, QEMU using scancode set 1 (from empirical testing)
 */
extern const char keyboard_scancode_1_to_ascii_map[256];
extern const char keyboard_scancode_1_to_ascii_map_shifted[256];

/**
 * KeyboardDriverState - Contain all driver states
 *
 * @param read_extended_mode Optional, can be used for signaling next read is
 * extended scancode (ex. arrow keys)
 * @param keyboard_input_on  Indicate whether keyboard ISR is activated or not
 * @param keyboard_buffer    Storing keyboard input values in ASCII
 */
struct KeyboardDriverState {
	bool read_extended_mode;
	bool keyboard_input_on;
	bool buffer_filled;
	char keyboard_buffer;

	bool shift_modifier;
	bool alt_modifier;
	bool ctrl_modifier;
} __attribute((packed));

extern struct KeyboardDriverState keyboard_state;

/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void);

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void);

// Get keyboard buffer value and flush the buffer - @param buf Pointer to char
// buffer
void get_keyboard_buffer(char *buf);

/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 */
void keyboard_isr(void);

#endif
