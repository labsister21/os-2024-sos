#include "text/framebuffer.h"
#include "cpu/portio.h"
#include "text/buffercolor.h"
#include <std/stdbool.h>
#include <std/stdint.h>

struct FramebufferState framebuffer_state = {
		.row = 0, .col = 0, .fg = WHITE, .bg = BLACK
};

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
	uint16_t pos = r * BUFFER_WIDTH + c;
	out(CURSOR_PORT_CMD, 0x0F);
	out(CURSOR_PORT_DATA, pos & 0xFF);

	out(CURSOR_PORT_CMD, 0x0E);
	out(CURSOR_PORT_DATA, (pos >> 8) & 0xFF);

	framebuffer_state.row = r;
	framebuffer_state.col = c;
}

void framebuffer_put(char c) {
	framebuffer_write(
			framebuffer_state.row, framebuffer_state.col, c, framebuffer_state.fg,
			framebuffer_state.bg
	);

	uint8_t next_col = framebuffer_state.col;
	uint8_t next_row = framebuffer_state.row;

	next_col += 1;
	if (next_col == BUFFER_WIDTH) {
		next_col = 0;
		next_row += 1;
	}

	if (next_row == BUFFER_HEIGHT) {
		next_row = 0;
	}

	framebuffer_set_cursor(next_row, next_col);
}

void framebuffer_write(
		uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg
) {
	uint16_t flatten = (row * BUFFER_WIDTH) + col;
	uint16_t *target =
			(uint16_t *)(0xC0000000 + BUFFER_BASE + flatten * CHAR_PRINTER_SIZE);
	uint16_t attrib = (bg << 4) | (fg & 0x0F);
	*target = c | (attrib << 8);
}

void framebuffer_clear(void) {
	for (int i = 0; i < BUFFER_HEIGHT; ++i) {
		for (int j = 0; j < BUFFER_WIDTH; ++j) {
			framebuffer_write(i, j, '\0', WHITE, BLACK);
		}
	}
}

void framebuffer_put_hex(uint32_t value) {
	char c[8];

	for (int i = 7; i >= 0; --i) {
		int mod = value % 16;
		value /= 16;

		if (mod < 10) c[i] = mod + '0';
		else c[i] = (mod - 10) + 'A';
	}

	framebuffer_put('0');
	framebuffer_put('x');
	for (int i = 0; i < 8; ++i) {
		framebuffer_put(c[i]);
	}
};
