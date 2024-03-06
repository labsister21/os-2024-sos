#include "header/cpu/interrupt.h"
#include "header/cpu/gdt.h"
#include "header/cpu/idt.h"
#include "header/cpu/portio.h"
#include "header/driver/keyboard.h"
#include "header/filesystem/fat32.h"
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

void syscall(struct InterruptFrame *frame) {
  switch (frame->cpu.general.eax) {
  case 0:
    *((int8_t *)frame->cpu.general.ecx) =
        read((struct FAT32DriverRequest *)frame->cpu.general.ebx);
    break;

  case 4:
    keyboard_state_activate();
    __asm__ volatile("sti"); // Allow hardware interrupt
    while (true) {
      __asm__ volatile("hlt");
      if (keyboard_state.buffer_filled) {
        get_keyboard_buffer((char *)frame->cpu.general.ebx);
        break;
      }
    }
    keyboard_state_deactivate();
    break;

  case 5:
    framebuffer_state.fg = frame->cpu.general.ecx;
    framebuffer_put(*(char *)frame->cpu.general.ebx);
    break;
  }
}

void main_interrupt_handler(struct InterruptFrame frame) {
  if (false) { // Debug
    int n = frame.int_number;
    framebuffer_put((n / 10) + '0');
    framebuffer_put((n % 10) + '0');
    framebuffer_put(' ');
  }
  switch (frame.int_number) {
  case PIC1_OFFSET + IRQ_KEYBOARD:
    keyboard_isr();
    break;
  case SYSCALL_INT:
    syscall(&frame);
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