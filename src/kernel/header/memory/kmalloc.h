#ifndef _KMALLOC_H
#define _KMALLOC_H

#include <std/stdint.h>

void *kmalloc(uint32_t size);
void kfree(void *address);

#endif
