#include <stdint.h>
#include <stdbool.h>
#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/driver/framebuffer.h"
#include "header/driver/keyboard.h"
#include "header/cpu/portio.h"
#include "header/interrupt/idt.h"
#include "header/interrupt/interrupt.h"
#include "header/driver/disk.h"
#include "header/filesystem/fat32.h"

void kernel_setup(void) {
    load_gdt(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    char c[2] = {'1', '2'};
    struct FAT32DriverRequest r = {
        .buf = c,
        .buffer_size = 2,
        .ext = "pdf",
        .name = "con",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    int result = write(r);
    char c1 = result + '0';
    framebuffer_write(0,0, c1, 0xF, 0);

    //struct BlockBuffer b;
    //for (int i = 0; i < 512; i++) b.buf[i] = i % 16;
    //write_blocks(&b, 17, 1);
    while (true);
}
