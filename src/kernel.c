#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include <stdint.h>

void kernel_setup(void)
{
  uint32_t a;
  uint32_t b = 0x0000BABE;
  __asm__("mov $0xCAFE0000, %0" : "=r"(a));
  while (1)
    b += 1;
}
