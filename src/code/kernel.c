#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/fat32.h"
#include "header/kernel-entrypoint.h"
#include "header/memory/paging.h"
#include "header/stdlib/string.h"
#include "header/text/framebuffer.h"
#include <stdbool.h>
#include <stdint.h>

void kernel_setup(void) {
  load_gdt(&_gdt_gdtr);
  pic_remap();
  initialize_idt();
  activate_keyboard_interrupt();
  framebuffer_clear();
  framebuffer_set_cursor(0, 0);
  initialize_filesystem_fat32();

  gdt_install_tss();
  set_tss_register();

  void *mem = 0;
  paging_allocate_user_page_frame(&_paging_kernel_page_directory, mem);

  struct FAT32DriverRequest req;
  req.buf = mem;
  req.buffer_size = PAGE_FRAME_SIZE;
  req.parent_cluster_number = ROOT_CLUSTER_NUMBER;
  str_cpy(req.name, "shell", 8);
  str_cpy(req.ext, "", 3);
  read(&req);

  set_tss_kernel_current_stack();
  kernel_execute_user_program(mem);
  framebuffer_write(0, 0, 'k', WHITE, BLACK);

  while (1)
    continue;
}
