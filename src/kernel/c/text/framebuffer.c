#include "text/framebuffer.h"
#include "cpu/portio.h"
#include "memory/kmalloc.h"
#include "text/buffercolor.h"
#include <std/stdbool.h>
#include <std/stdint.h>
#include <std/string.h>

struct FramebufferState framebuffer_state = {
		.row = 0, .col = 0, .fg = WHITE, .bg = BLACK
};

typedef uint16_t Buffer[BUFFER_HEIGHT][BUFFER_WIDTH];

struct FramebufferLayer {
	Buffer buffer;
	struct FramebufferLayer *next;
	struct FramebufferLayer *prev;
};

struct FramebufferLayer *base_layer;

void framebuffer_initialize_base_layer() {
	struct FramebufferLayer *layer = kmalloc(sizeof(struct FramebufferLayer));
	layer->next = NULL;
	layer->prev = NULL;
	memset(layer->buffer, ' ', sizeof(Buffer));
	base_layer = layer;

	framebuffer_clear();
}

struct FramebufferLayer *framebuffer_create_layer() {
	struct FramebufferLayer *layer = kmalloc(sizeof(struct FramebufferLayer));
	if (layer == NULL)
		return NULL;

	layer->next = NULL;
	memset(layer->buffer, 0, sizeof(Buffer));

	struct FramebufferLayer *current = base_layer;
	while (current->next != NULL)
		current = current->next;
	current->next = layer;
	layer->prev = current;

	return layer;
}

void framebuffer_remove_layer(struct FramebufferLayer *layer) {
	struct FramebufferLayer *current = base_layer;
	while (current != NULL) {
		if (current == layer) break;
		current = current->next;
	}

	if (current == NULL) return;

	for (int row = 0; row < BUFFER_HEIGHT; ++row) {
		for (int col = 0; col < BUFFER_WIDTH; ++col) {
			if (layer->buffer[row][col] == '\0') continue;
			framebuffer_write_to_layer(layer, row, col, '\0');
		}
	}

	if (current->prev)
		current->prev->next = current->next;

	if (current->next)
		current->next->prev = current->prev;

	kfree(layer);
}

void framebuffer_write_to_layer(struct FramebufferLayer *layer, int row, int col, char c) {
	if (c == '\0') {
		// Search upward layer
		struct FramebufferLayer *current = layer->next;
		bool overided = false;
		while (current != NULL) {
			if (current->buffer[row][col] != '\0') {
				overided = true;
				break;
			};
			current = current->next;
		};

		if (!overided) {
			current = layer->prev;
			while (current != NULL) {
				if (current->buffer[row][col] != '\0') break;
				current = current->prev;
			}
			framebuffer_write(row, col, current->buffer[row][col], framebuffer_state.fg, framebuffer_state.bg);
		}

		layer->buffer[row][col] = '\0';
	} else {
		struct FramebufferLayer *current = layer->next;
		while (current != NULL) {
			if (current->buffer[row][col] != '\0') break;
			current = current->next;
		}
		if (current == NULL)
			framebuffer_write(row, col, c, framebuffer_state.fg, framebuffer_state.bg);
		layer->buffer[row][col] = c;
	}
}

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

void framebuffer_scroll_up() {
	static bool scrolling = false;

	if (scrolling)
		return;

	scrolling = true;
	for (int row = 0; row < BUFFER_HEIGHT - 1; ++row) {
		for (int col = 0; col < BUFFER_WIDTH; ++col) {

			uint16_t flatten;

			flatten = ((row + 1) * BUFFER_WIDTH) + col;
			uint16_t value = *(uint16_t *)(0xC0000000 + BUFFER_BASE + flatten * CHAR_PRINTER_SIZE);

			flatten = (row * BUFFER_WIDTH) + col;
			uint16_t *address = (uint16_t *)(0xC0000000 + BUFFER_BASE + flatten * CHAR_PRINTER_SIZE);
			*address = value;
		}
	}

	for (int col = 0; col < BUFFER_WIDTH; ++col) {
		framebuffer_write(BUFFER_HEIGHT - 1, col, ' ', WHITE, BLACK);
	}
	scrolling = false;
}

void framebuffer_next_line(void) {
	int next_row = framebuffer_state.row;
	next_row += 1;
	if (next_row == BUFFER_HEIGHT) {
		framebuffer_scroll_up();
		next_row = BUFFER_HEIGHT - 1;
	}
	framebuffer_set_cursor(next_row, 0);
};

void framebuffer_put(char c) {
	base_layer->buffer[framebuffer_state.row][framebuffer_state.col] = c;
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
		framebuffer_scroll_up();
		next_row = BUFFER_HEIGHT - 1;
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
	for (int row = 0; row < BUFFER_HEIGHT; ++row) {
		for (int col = 0; col < BUFFER_WIDTH; ++col) {
			framebuffer_write_to_layer(base_layer, row, col, ' ');
		}
	}
	framebuffer_set_cursor(0, 0);
}

void framebuffer_puts(char *str) {
	int i = 0;
	while (str[i] != '\0')
		framebuffer_put(str[i++]);
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
