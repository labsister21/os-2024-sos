#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include "header/cpu/portio.h"
#include "header/idt.h"
#include "header/interrupt.h"

void kernel_setup(void) {
    // uint32_t a;
    // uint32_t volatile b = 0x0000BABE;
    // __asm__("mov $0xCAFE0000, %0" : "=r"(a));
    // while (true) b += 1;
    // load_gdt(&_gdt_gdtr);
    // while (true);
    // framebuffer_clear();
    // framebuffer_write(3, 8,  'H', 0, 0xF);
    // framebuffer_write(3, 9,  'a', 0, 0xF);
    // framebuffer_write(3, 10, 'i', 0, 0xF);
    // framebuffer_write(3, 11, 'k', 0, 0xF);
    // framebuffer_write(3, 12, 'a', 0, 0xF);
    // framebuffer_write(3, 13, 'l', 0, 0xF);
    // framebuffer_set_cursor(3, 10);
    // while (true);
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    __asm__("int $0x4");
    while (true);

}
