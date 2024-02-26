#include "header/cpu/gdt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/buffercolor.h"
#include "header/text/framebuffer.h"

void kernel_setup(void) {
  load_gdt(&_gdt_gdtr);
  for (int i = 0; i < 25; ++i) {
    for (int j = 0; j < 80; ++j) {
      framebuffer_write(i, j, (j % 10) + '0', WHITE, BLACK);
    }
  }
  while (1) continue;
}
