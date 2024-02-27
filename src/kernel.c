#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/kernel-entrypoint.h"
#include "header/text/buffercolor.h"
#include "header/text/framebuffer.h"

void kernel_setup(void) {
  load_gdt(&_gdt_gdtr);
  pic_remap();
  initialize_idt();
  // framebuffer_clear();
  __asm__("int $0x4");
  while (1)
    continue;
}
