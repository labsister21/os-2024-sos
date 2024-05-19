#include "cpu/interrupt.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/portio.h"
#include "driver/keyboard.h"
#include "driver/time.h"
#include "driver/tty.h"
#include "filesystem/fat32.h"
#include "filesystem/vfs.h"
#include "process/file_descriptor.h"
#include "process/scheduler.h"
#include "text/buffercolor.h"
#include "text/framebuffer.h"
#include <std/string.h>
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

struct InterruptFrame *current_interrupt_frame;

bool syscall_return_value_flag = false;
void syscall_handler(struct InterruptFrame *frame) {
	current_interrupt_frame = frame;
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
	int *return_value = (void *)(&frame->cpu.general.eax);
	uint32_t first = frame->cpu.general.ebx;
	uint32_t second = frame->cpu.general.ecx;
	uint32_t third = frame->cpu.general.edx;

	uint32_t result;
	switch (frame->cpu.general.eax) {
	case GET_CHAR: {
		char *ptr = (char *)first;
		*ptr = fgetc();
	} break;

	case GET_CHAR_NON_BLOCKING: {
		char *ptr = (char *)first;
		get_keyboard_buffer(ptr);
	} break;

	case PUT_CHAR: {
		fputc((char)first);
	} break;

	case FRAMEBUFFER_PUT_CHAR:
		framebuffer_put((char)first);
		break;

	case FRAMEBUFFER_PUT_CHARS: {
		int i = second;
		char *str = (char *)first;
		while (i--) {
			framebuffer_put(str[i]);
			++i;
		}
	} break;

	case FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS: {
		int i = 0;
		char *str = (char *)first;
		while (str[i] != '\0') {
			framebuffer_put(str[i]);
			++i;
		}
	} break;

	case FRAMEBUFFER_CLEAR:
		framebuffer_clear();
		break;

	case FRAMEBUFFER_CURSOR:
		framebuffer_set_cursor(second, first);
		break;

	case GET_TIME:
		memcpy((void *)first, &startup_time, sizeof(struct TimeRTC));
		break;

	case EXEC: {
		result = process_create((char *)first);
	} break;

	case KILL: {
		result = process_destroy((int)first);
	} break;

	case EXIT: {
		int pid = get_current_running_pid();
		scheduler_handle_timer_interrupt(frame);
		int new_pid = get_current_running_pid();

		if (pid == new_pid) {
			result = -1;
			break;
		}

		result = process_destroy(pid);
	} break;

	case VFS_STAT: {
		result = vfs.stat((char *)first, (struct VFSEntry *)second);
	} break;

	case VFS_DIR_STAT: {
		result = vfs.dirstat((char *)first, (struct VFSEntry *)second);
	} break;

	case VFS_MKDIR: {
		result = vfs.mkdir((char *)first);
	} break;

	case VFS_MKFILE: {
		result = vfs.mkfile((char *)first);
	} break;

	case VFS_OPEN: {
		int fd = get_free_fd_of_current_process();
		if (fd < 0) {
			result = fd;
			break;
		}
		int ft = vfs.open((char *)first);
		if (ft < 0) {
			result = ft;
			break;
		}
		set_ft_of_current_process(fd, ft);
		result = fd;
	} break;

	case VFS_CLOSE: {
		int fd = (int)first;
		int ft = get_ft_of_current_process(fd);
		result = vfs.close(ft);
		if (result == 0)
			clear_fd_of_current_process(fd);
	} break;

	case VFS_READ: {
		int fd = (int)first;
		int ft = get_ft_of_current_process(fd);
		result = vfs.read(ft, (char *)second, (int)third);
	} break;

	case VFS_WRITE: {
		int fd = (int)first;
		int ft = get_ft_of_current_process(fd);
		result = vfs.write(ft, (char *)second, (int)third);
	} break;

	default: {
		framebuffer_puts("System call not implemented");
		result = -1;
	} break;
	}

	if (syscall_return_value_flag)
		*return_value = result;
}

void main_interrupt_handler(struct InterruptFrame frame) {
	if (true) { // Debug
		int n = frame.int_number;
		framebuffer_write(24, 68, (n / 10) + '0', WHITE, BLACK);
		framebuffer_write(24, 69, (n % 10) + '0', WHITE, BLACK);
	}

	switch (frame.int_number) {
	case 14: { // Page fault
		int pid = get_current_running_pid();
		scheduler_handle_timer_interrupt(&frame);
		process_destroy(pid);
	} break;
	case PIC1_OFFSET + IRQ_TIMER: // Timer
		time_handle_timer_interrupt();
		scheduler_handle_timer_interrupt(&frame);
		break;
	case PIC1_OFFSET + IRQ_KEYBOARD:
		keyboard_isr();
		break;
	case PIC1_OFFSET + IRQ_CMOS:
		handle_rtc_interrupt();
		break;
	case SYSCALL_INT:
		syscall_return_value_flag = true;
		syscall_handler(&frame);
		break;
	default:
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
