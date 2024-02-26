#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include <stdint.h>

void kernel_setup(void) {
  load_gdt(&_gdt_gdtr);
  while (1) continue;
}
