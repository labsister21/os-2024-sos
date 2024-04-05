#include "cpu/interrupt.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/portio.h"
#include "driver/keyboard.h"
#include "filesystem/fat32.h"
#include "text/buffercolor.h"
#include "text/framebuffer.h"
#include <syscall.h>

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

void syscall_handler(struct InterruptFrame *frame) {
	switch (frame->cpu.general.eax) {
	case READ:
		*((int8_t *)frame->cpu.general.ecx) = read((struct FAT32DriverRequest *)frame->cpu.general.ebx);
		break;

	case READ_DIRECTORY:
		*((int8_t *)frame->cpu.general.ecx) = read_directory((struct FAT32DriverRequest *)frame->cpu.general.ebx);
		break;

	case WRITE:
		*((int8_t *)frame->cpu.general.ecx) = write((struct FAT32DriverRequest *)frame->cpu.general.ebx);
		break;

	case GET_CHAR:
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

	case FRAMEBUFFER_PUT_CHAR:
		framebuffer_put((char)frame->cpu.general.ebx);
		break;

	case FRAMEBUFFER_PUT_CHARS: {
		int i = frame->cpu.general.ecx;
		char *str = (char *)frame->cpu.general.ebx;
		while (i--) {
			framebuffer_put(str[i]);
			++i;
		}
	} break;

	case FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS: {
		int i = 0;
		char *str = (char *)frame->cpu.general.ebx;
		while (str[i] != '\0') {
			framebuffer_put(str[i]);
			++i;
		}
	} break;

	case FRAMEBUFFER_CLEAR:
		framebuffer_clear();
		break;

	case FRAMEBUFFER_CURSOR:
		framebuffer_set_cursor(frame->cpu.general.ecx, frame->cpu.general.ebx);
		break;
	}
}

void main_interrupt_handler(struct InterruptFrame frame) {
	if (true) { // Debug
		int n = frame.int_number;
		framebuffer_write(24, 0, (n / 10) + '0', WHITE, BLACK);
		framebuffer_write(24, 1, (n % 10) + '0', WHITE, BLACK);
	}
	switch (frame.int_number) {
	case PIC1_OFFSET + IRQ_KEYBOARD:
		keyboard_isr();
		break;
	case SYSCALL_INT:
		syscall_handler(&frame);
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
