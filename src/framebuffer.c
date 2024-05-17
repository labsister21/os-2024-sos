#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t pos = r * 80 + c;
 
	out(0x3D4, 0x0F);
	out(0x3D5, (uint8_t) (pos & 0xFF));
	out(0x3D4, 0x0E);
	out(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // TODO : Implement
    uint16_t pos = row*80 + col;
    uint8_t bgcolor = (bg << 4) | (fg & 0x0F); 
    uint16_t value = ((uint16_t)bgcolor << 8) | c;
    uint16_t *fb = (uint16_t *) FRAMEBUFFER_MEMORY_OFFSET;
    fb[pos] = value;
}

void framebuffer_clear(void) {
    // TODO : Implement
    uint16_t *fb = (uint16_t *) FRAMEBUFFER_MEMORY_OFFSET; // Mengambil alamat memory framebuffer
    // Looping untuk menghapus semua karakter yang ada di framebuffer
    for (uint16_t i = 0; i < 80 * 25; i++) {
        fb[i] = (0x07 << 8) | 0x00;
    }
}