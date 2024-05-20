#ifndef _BOOT_H
#define _BOOT_H

#include "boot/multiboot.h"
#include <std/stdint.h>

/**
 * Multiboot info address should be at ebx register
 */
void init_multiboot_info();

multiboot_info_t *get_multiboot_info();

uint32_t get_memory_size();

#endif
