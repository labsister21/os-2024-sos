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
        .buffer_size = 1000,
        .ext = "pdf",
        .name = "con",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    int result = write(r);
    char c1 = result + '0';
    framebuffer_write(0,0, c1, 0xF, 0);
    char c3[4] =  {'a','s','e','p'};
    struct FAT32DriverRequest r2 = {
        .buf = c3,
        .buffer_size = 10000,
        .ext = "pdf",
        .name = "hoho",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    int res = write(r2);
    char c2 = res + '0';
    framebuffer_write(0,2, c2, 0xF, 0);
    char p[11];
    struct FAT32DriverRequest r3 = {
        .buf = p,
        .buffer_size = 10000,
        .ext = "pdf",
        .name = "con",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    int ress = delete(r3);
    char c4 = ress + '0';
    framebuffer_write(4,1,c4, 0xF, 0);
    char q[11] = {'h','a','i',' ','o','n','i','c','a','n'};
    struct FAT32DriverRequest r4 = {
        .buf = q,
        .buffer_size = 10000,
        .ext = "pdf",
        .name = "niinii",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    int ress2 = write(r4);
    char pq = ress2 + '0';
    char s[11];
    struct FAT32DriverRequest r6 = {
        .buf = s,
        .buffer_size = 10000,
        .ext = "pdf",
        .name = "niinii",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    int ress5 = read(r6);
    char ps = ress5 + '0';
    framebuffer_write(5,0,ps,0xF,0);
    char o[4] = {'h','a', 'l', 'o'};
    struct FAT32DriverRequest r5 = {
        .buf = o,
        .buffer_size = 0,
        .name = "niinii",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    int ress6 = write(r5);
    void *so;
    struct FAT32DriverRequest r7 = {
        .buf = so,
        .buffer_size = 0,
        .name = "niinii",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    int ress7 = read_directory(r7);
    char soo[4];
    struct FAT32DriverRequest r8 = {
        .buf = soo,
        .buffer_size = 0,
        .name = "tes",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    };
    write(r8);
    char sooo[4];
    struct FAT32DriverRequest r9 = {
        .buf = sooo,
        .buffer_size = 0,
        .name = "inDir",
        .parent_cluster_number = 13,
    };
    int pl = write(r9);
    char aq =  pl + '0';
    struct FAT32DirectoryTable *k;
    struct FAT32DriverRequest r10 = {
        .buf = k,
        .buffer_size = 0,
        .name = "jacko",
        .parent_cluster_number = 15,
    };
    int nig = write(r10);
    int del = read(r10);
    framebuffer_write(24,0, del + '0', 0xF, 0);
    framebuffer_write(11,20, aq,0xF,0);

    framebuffer_write(10,3,s[0],0xF,0);
    framebuffer_write(2,4,s[1],0xF,0);
    framebuffer_write(2,5,s[2],0xF,0);
    framebuffer_write(2,6,s[3],0xF,0);
    framebuffer_write(2,7,s[4],0xF,0);
    framebuffer_write(2,3,s[0],0xF,0);
    framebuffer_write(2,4,s[1],0xF,0);
    framebuffer_write(2,5,s[2],0xF,0);
    framebuffer_write(2,6,s[3],0xF,0);
    framebuffer_write(2,7,s[4],0xF,0);
    framebuffer_write(2,8,s[5],0xF,0);
    framebuffer_write(2,9,s[6],0xF,0);
    framebuffer_write(2,10,s[7],0xF,0);
    framebuffer_write(2,11,s[8],0xF,0);
    framebuffer_write(2,12,s[9],0xF,0);
    framebuffer_write(2,13,s[10],0xF,0);
    

    //struct BlockBuffer b;
    //for (int i = 0; i < 512; i++) b.buf[i] = i % 16;
    //write_blocks(&b, 17, 1);
    while (true);
}
