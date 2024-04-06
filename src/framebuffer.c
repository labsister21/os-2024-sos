#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/string.h"
#include "header/driver/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
    // TODO : Implement
    uint16_t cursor_position = (r * 80) + c;
    out16(0x3D4, 0x0F);
    out16(0x3D5, (uint8_t)(cursor_position & 0xFF));
    out16(0x3D4, 0x0E);
    out16(0x3D5, (uint8_t)((cursor_position >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    // TODO : Implement
    uint16_t attribute = (bg << 4) | (fg & 0x0F);
    volatile uint16_t *position;
    position = (volatile uint16_t *)0xB8000 + (row * 80 + col);
    *position = c | (attribute << 8);
}

void framebuffer_clear(void)
{
    // TODO : Implement
    memset(FRAMEBUFFER_MEMORY_OFFSET, 0x00, 80 * 25 * sizeof(char));
}
