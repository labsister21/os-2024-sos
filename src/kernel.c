#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/driver/framebuffer.h"
#include "header/driver/keyboard.h"
#include "header/cpu/portio.h"
#include "header/interrupt/idt.h"
#include "header/interrupt/interrupt.h"
#include "disk.h"

// void kernel_setup(void)
// {
//     uint16_t p[102031203];
//     load_gdt(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);

//     int col = 0;
//     keyboard_state_activate();
//     while (true)
//     {
//         char c;
//         get_keyboard_buffer(&c);
//         if (c)
//             framebuffer_write(0, col++, c, 0xF, 0);
//     }
// }

void kernel_setup(void)
{
    framebuffer_write(0, 0, 'f', 1, 2);
    load_gdt(&_gdt_gdtr);
    framebuffer_write(0, 1, 'e', 1, 2);
    pic_remap();
    framebuffer_write(0, 2, 'b', 1, 2);
    activate_keyboard_interrupt();
    framebuffer_write(0, 3, 'r', 1, 2);

    initialize_idt();
    framebuffer_write(0, 4, 'y', 1, 2);

    // framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    // framebuffer_write(0, 2, 'f', 1, 2);

    struct BlockBuffer bb;
    for (int i = 0; i < 512; i++)
    {
        bb.buf[i] = i % 16;
    }
    write_blocks(&bb, 0, 1);
    // framebuffer_write(0, 2, 'g', 1, 2);

    while (true)
        ;
}
