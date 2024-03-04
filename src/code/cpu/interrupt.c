#include "header/cpu/interrupt.h"
#include "header/cpu/gdt.h"
#include "header/cpu/portio.h"
#include "header/driver/keyboard.h"
#include "header/text/buffercolor.h"
#include "header/text/framebuffer.h"

void activate_keyboard_interrupt(void) {
  out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_KEYBOARD));
};

void io_wait(void) { out(0x80, 0); }

void pic_ack(uint8_t irq) {
  if (irq >= 8) out(PIC2_COMMAND, PIC_ACK);
  out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
  // Starts the initialization sequence in cascade mode
  out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();

  // ICW2: Master PIC vector offset
  out(PIC1_DATA, PIC1_OFFSET);
  io_wait();

  // ICW2: Slave PIC vector offset
  out(PIC2_DATA, PIC2_OFFSET);
  io_wait();

  // ICW3: tell Master PIC, slave PIC at IRQ2 (0000 0100)
  out(PIC1_DATA, 0b0100);
  io_wait();

  // ICW3: tell Slave PIC its cascade identity (0000 0010)
  out(PIC2_DATA, 0b0010);
  io_wait();

  out(PIC1_DATA, ICW4_8086);
  io_wait();
  out(PIC2_DATA, ICW4_8086);
  io_wait();

  // Disable all interrupts
  out(PIC1_DATA, PIC_DISABLE_ALL_MASK);
  out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

int c = 0;
void main_interrupt_handler(struct InterruptFrame frame) {
  int p = frame.int_number;
  framebuffer_write(10, c++, (p / 10) + '0', WHITE, BLACK);
  framebuffer_write(10, c++, (p % 10) + '0', WHITE, BLACK);
  framebuffer_write(10, c++, ' ', WHITE, BLACK);
  switch (frame.int_number) {
  case PIC1_OFFSET + IRQ_KEYBOARD:
    keyboard_isr();
    break;
  }
};

struct TSSEntry _interrupt_tss_entry = {
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void set_tss_kernel_current_stack(void) {
  uint32_t stack_ptr;
  // Reading base stack frame instead esp
  __asm__ volatile("mov %%ebp, %0" : "=r"(stack_ptr) : /* <Empty> */);
  // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
  _interrupt_tss_entry.esp0 = stack_ptr + 8;
}