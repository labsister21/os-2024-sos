#include "header/text/framebuffer.h"
#include "header/cpu/portio.h"
#include "header/stdlib/string.h"
#include "header/text/buffercolor.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// void framebuffer_set_cursor(uint8_t r, uint8_t c) {
//   // TODO : Implement
// }

void framebuffer_write(
    uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg
) {
  uint16_t flatten = (row * BUFFER_WIDTH) + col;
  uint16_t *target = (uint16_t *)(BUFFER_BASE + flatten * CHAR_PRINTER_SIZE);
  uint16_t attrib = (bg << 4) | (fg & 0x0F);
  *target = c | (attrib << 8);
}

void framebuffer_clear(void) {
  for (int i = 0; i < BUFFER_HEIGHT; ++i) {
    for (int j = 0; j < BUFFER_WIDTH; ++j) {
      framebuffer_write(i, j, ' ', BLACK, BLACK);
    }
  }
}
