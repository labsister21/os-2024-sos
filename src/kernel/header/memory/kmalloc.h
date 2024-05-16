#ifndef _KMALLOC_H
#define _KMALLOC_H

#include <std/stdint.h>

void *kmalloc(uint32_t size);
void *kmalloc_aligned(uint32_t size, uint32_t align);
void kfree(void *address);

// void ktest();

#endif
