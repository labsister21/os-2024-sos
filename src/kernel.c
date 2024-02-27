#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/interrupt.h"
#include "header/driver/keyboard.h"
#include "header/kernel-entrypoint.h"
#include "header/text/buffercolor.h"
#include "header/text/framebuffer.h"

void kernel_setup(void) {
  load_gdt(&_gdt_gdtr);
  pic_remap();
  activate_keyboard_interrupt();
  initialize_idt();

  int col = 0;
  int row = 0;
  keyboard_state_activate();
  while (true) {
    char c;
    get_keyboard_buffer(&c);
    if (c == 0) continue;

    if (c == '\n') {
      row += 1;
      col = 0;
      framebuffer_set_cursor(row, col);
      continue;
    }

    framebuffer_write(row, col++, c, BLACK, WHITE);
    framebuffer_set_cursor(row, col);
    if (col % 80 == 0) {
      col = 0;
      ++row;
    }
  }
}
