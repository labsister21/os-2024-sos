#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include "text/buffercolor.h"
#include <std/stdbool.h>
#include <std/stdint.h>

#define CURSOR_PORT_CMD 0x03D4
#define CURSOR_PORT_DATA 0x03D5

#define BUFFER_BASE 0xB8000
#define BUFFER_WIDTH 80
#define BUFFER_HEIGHT 25
#define CHAR_PRINTER_SIZE 2

struct FramebufferState {
	int row;
	int col;
	int fg;
	int bg;
} __attribute((packed));

/**
 * Terminal framebuffer
 * Resolution: 80x25
 * Starting at FRAMEBUFFER_MEMORY_OFFSET,
 * - Even number memory: Character, 8-bit
 * - Odd number memory:  Character color lower 4-bit, Background color upper
 * 4-bit
 */

/**
 * Set framebuffer character and color with corresponding parameter values.
 * More details: https://en.wikipedia.org/wiki/BIOS_color_attributes
 *
 * @param row Vertical location (index start 0)
 * @param col Horizontal location (index start 0)
 * @param c   Character
 * @param fg  Foreground / Character color
 * @param bg  Background color
 */
void framebuffer_write(
		int row, int col, char c, int fg, int bg
);

/**
 * Set cursor to specified location. Row and column starts from 0
 *
 * @param r row
 * @param c column
 */
void framebuffer_set_cursor(int row, int col);

/**
 * Set all cell in framebuffer character to 0x00 (empty character)
 * and color to 0x07 (gray character & black background)
 * Extra note: It's allowed to use different color palette for this
 *
 */
void framebuffer_clear(void);

void framebuffer_initialize_base_layer();
struct FramebufferLayer *framebuffer_create_layer();
void framebuffer_remove_layer(struct FramebufferLayer *layer);
void framebuffer_write_to_layer(struct FramebufferLayer *layer, int row, int col, char c);

enum FramebufferCursorMove {
	UP,
	RIGHT,
	DOWN,
	LEFT
};

void framebuffer_move_cursor(enum FramebufferCursorMove direction, int count);
void framebuffer_next_line(void);
void framebuffer_put(char c);
void framebuffer_puts(char *c);
void framebuffer_put_hex(uint32_t value);

#endif
