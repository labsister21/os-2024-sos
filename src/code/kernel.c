#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/fat32.h"
#include "header/kernel-entrypoint.h"
#include "header/memory/paging.h"
#include "header/text/framebuffer.h"
#include <stdbool.h>
#include <stdint.h>

void kernel_setup(void) {
  load_gdt(&_gdt_gdtr);
  pic_remap();
  activate_keyboard_interrupt();
  keyboard_state_activate();
  initialize_idt();
  initialize_filesystem_fat32();
  framebuffer_clear();

  paging_allocate_user_page_frame(
      &_paging_kernel_page_directory, (void *)0x500000
  );
  // paging_free_user_page_frame(&_paging_kernel_page_directory, (void
  // *)0x500000);
  *((uint8_t *)0x500000) = 1;

  while (1) continue;
}
