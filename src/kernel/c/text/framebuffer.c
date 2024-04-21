#include "text/framebuffer.h"
#include "cpu/portio.h"
#include "text/buffercolor.h"
#include <std/stdbool.h>
#include <std/stdint.h>

struct FramebufferState framebuffer_state = {
		.row = 0, .col = 0, .fg = WHITE, .bg = BLACK
};

void framebuffer_set_cursor(int row, int col) {
	uint16_t pos = row * BUFFER_WIDTH + col;
	out(CURSOR_PORT_CMD, 0x0F);
	out(CURSOR_PORT_DATA, pos & 0xFF);

	out(CURSOR_PORT_CMD, 0x0E);
	out(CURSOR_PORT_DATA, (pos >> 8) & 0xFF);

	if (row != -1) {
		if (row < 0) row = 0;
		if (row > BUFFER_HEIGHT - 1) row = BUFFER_HEIGHT - 1;
	} else {
		row = framebuffer_state.row;
	}

	if (col != -1) {
		if (col < 0) col = 0;
		if (col > BUFFER_WIDTH - 1) col = BUFFER_WIDTH - 1;
	} else {
		col = framebuffer_state.col;
	}

	framebuffer_state.row = row;
	framebuffer_state.col = col;
}

void framebuffer_next_line(void) {
	int next_row = framebuffer_state.row;
	next_row += 1;
	if (next_row == BUFFER_HEIGHT) {
		next_row = 0;
	}
	framebuffer_set_cursor(next_row, 0);
};

void framebuffer_move_cursor(enum FramebufferCursorMove direction, int count) {
	int next_row = framebuffer_state.row;
	int next_col = framebuffer_state.col;
	switch (direction) {
	case UP: {
		next_row -= count;
	} break;
	case DOWN: {
		next_row += count;
	} break;
	case LEFT: {
		next_col -= count;
	} break;
	case RIGHT: {
		next_col += count;
	} break;
	}
	framebuffer_set_cursor(next_row, next_col);
};

void framebuffer_put(char c) {
	framebuffer_write(
			framebuffer_state.row, framebuffer_state.col, c, framebuffer_state.fg,
			framebuffer_state.bg
	);

	int next_col = framebuffer_state.col;
	int next_row = framebuffer_state.row;

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
		int row, int col, char c, int fg, int bg
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
	framebuffer_set_cursor(0, 0);
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
