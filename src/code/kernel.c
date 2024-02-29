#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/fat32.h"
#include "header/kernel-entrypoint.h"
#include "header/stdlib/string.h"
#include "header/text/buffercolor.h"
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

  struct FAT32DriverRequest req;
  req.buf = "Afif";
  req.buffer_size = str_len(req.buf);
  str_cpy(req.name, "test", 8);
  str_cpy(req.ext, "x", 3);
  req.parent_cluster_number = ROOT_CLUSTER_NUMBER;

  write(&req);

  str_cpy(req.name, "dir", 8);
  req.buffer_size = 0;
  write(&req);

  struct FAT32DirectoryTable dir_table;
  req.buffer_size = CLUSTER_SIZE;
  req.buf = &dir_table;
  read_directory(&req);

  uint32_t dir_cluster = get_cluster_from_dir_entry(&dir_table.table[0]);
  req.parent_cluster_number = dir_cluster;
  str_cpy(req.name, "hello", 8);
  str_cpy(req.ext, "o", 3);
  req.buf = "HELLO, WORLD!";
  req.buffer_size = str_len(req.buf);
  write(&req);

  delete (&req);

  req.parent_cluster_number = ROOT_CLUSTER_NUMBER;
  str_cpy(req.name, "dir", 8);
  int res = delete (&req);
  framebuffer_write(0, 0, '0' + res, WHITE, BLACK);

  while (1) continue;
}
