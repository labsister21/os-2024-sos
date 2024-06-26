#ifndef _IDT_H
#define _IDT_H

#include <std/stdbool.h>
#include <std/stddef.h>
#include <std/stdint.h>

// IDT hard limit, see Intel x86 manual 3a - 6.10 Interrupt Descriptor Table
#define IDT_MAX_ENTRY_COUNT 256
#define ISR_STUB_TABLE_LIMIT 64

#define INTERRUPT_AND_TRAP_TYPE_ID 0b000
#define INTERRUPT_GATE_TYPE_AND_SIZE 0b01110

// Interrupt Handler / ISR stub for reducing code duplication, this array can be
// iterated in initialize_idt()
extern void *isr_stub_table[ISR_STUB_TABLE_LIMIT];

extern struct IDTR _idt_idtr;

/**
 * IDTGate, IDT entry that point into interrupt handler
 * Struct defined exactly in Intel x86 Vol 3a - Figure 6-2. IDT Gate Descriptors
 *
 * @param offset_low  Lower 16-bit offset
 * @param segment     Memory segment
 * @param _reserved   Reserved bit, bit length: 5
 * @param _r_bit_1    Reserved for idtgate type, bit length: 3
 * @param _r_bit_2    Reserved for idtgate type, bit length: 3
 * @param gate_32     Is this gate size 32-bit? If not then its 16-bit gate
 * @param _r_bit_3    Reserved for idtgate type, bit length: 1
 * ...
 */
struct IDTGate {
	/* Bit 0 to 31 */
	uint16_t offset_low;
	uint16_t segment_selector;

	/* Bit 32 to 47 */
	uint8_t : 5;								 // Reserved
	uint8_t type_identifier : 3; // Fill with 0b000 for Interrupt or Trap gate

	// 0b00101 for task gate
	// 0b0D110 for interrupt gate
	// 0b0D111 for trap gate
	// where D is size of gate
	uint8_t gate_type_and_size : 5;
	uint8_t dpl : 2; // Privilege Level
	uint8_t segment_present : 1;

	/* Bit 48 to 63 */
	uint16_t offset_hi;
} __attribute__((packed));

/**
 * Interrupt Descriptor Table, containing lists of IDTGate.
 * One IDT already defined in idt.c
 *
 * ...
 */
struct InterruptDescriptorTable {
	struct IDTGate table[IDT_MAX_ENTRY_COUNT];
} __attribute__((packed));

/**
 * IDTR, carrying information where's the IDT located and size.
 * Global kernel variable defined at idt.c.
 *
 * ...
 */
struct IDTR {
	uint16_t size;
	struct InterruptDescriptorTable *address;
} __attribute__((packed));

/**
 * Set IDTGate with proper interrupt handler values.
 * Will directly edit global IDT variable and set values properly
 *
 * @param int_vector       Interrupt vector to handle
 * @param handler_address  Interrupt handler address
 * @param gdt_seg_selector GDT segment selector, for kernel use
 * GDT_KERNEL_CODE_SEGMENT_SELECTOR
 * @param privilege        Descriptor privilege level
 */
void set_interrupt_gate(
		uint8_t int_vector, void *handler_address, uint16_t gdt_seg_selector,
		uint8_t privilege
);

/**
 * Set IDT with proper values and load with lidt
 */
void initialize_idt(void);

#endif
