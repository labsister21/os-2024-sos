#include "header/cpu/idt.h"
#include "header/cpu/gdt.h"
#include "header/cpu/interrupt.h"
#include <stdint.h>

struct InterruptDescriptorTable interrupt_descriptor_table;
struct IDTR _idt_idtr = {.size = sizeof(interrupt_descriptor_table),
                         .address = &interrupt_descriptor_table};

void initialize_idt(void) {
  for (int i = 0; i < ISR_STUB_TABLE_LIMIT; ++i) {
    set_interrupt_gate(i, isr_stub_table[i], GDT_KERNEL_CODE_SEGMENT_SELECTOR,
                       0);
  }
  __asm__ volatile("lidt %0" : : "m"(_idt_idtr));
  __asm__ volatile("sti");
};

void set_interrupt_gate(uint8_t int_vector, void *handler_address,
                        uint16_t gdt_seg_selector, uint8_t privilege) {
  struct IDTGate *idt_int_gate = &interrupt_descriptor_table.table[int_vector];

  uint32_t addr = (uint32_t)handler_address;

  idt_int_gate->offset_low = (uint16_t)(addr & 0xFFFF);
  idt_int_gate->segment_selector = gdt_seg_selector;
  idt_int_gate->type_identifier = INTERRUPT_AND_TRAP_TYPE_ID;
  idt_int_gate->gate_type_and_size = INTERRUPT_GATE_TYPE_AND_SIZE;
  idt_int_gate->dpl = privilege;
  idt_int_gate->segment_present = 1;
  idt_int_gate->offset_hi = (uint16_t)((addr >> 16) & 0xFFFF);
};
