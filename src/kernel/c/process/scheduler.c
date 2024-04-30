#include "process/scheduler.h"
#include "cpu/portio.h"
#include "kernel-entrypoint.h"
#include "text/framebuffer.h"
#include <std/stdint.h>
#include <std/string.h>

extern void kernel_start_user_mode(struct InterruptFrame *);

void activate_timer_interrupt(void) {
	__asm__ volatile("cli");
	// Setup how often PIT fire
	uint32_t pit_timer_counter_to_fire = PIT_TIMER_COUNTER;
	out(PIT_COMMAND_REGISTER_PIO, PIT_COMMAND_VALUE);
	out(PIT_CHANNEL_0_DATA_PIO, (uint8_t)(pit_timer_counter_to_fire & 0xFF));
	out(PIT_CHANNEL_0_DATA_PIO, (uint8_t)((pit_timer_counter_to_fire >> 8) & 0xFF));

	// Activate the interrupt
	out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_TIMER));
}

void scheduler_handle_timer_interrupt(struct InterruptFrame *frame) {
	pic_ack(PIC1_OFFSET + IRQ_TIMER);

	int current = 0;
	while (current < PROCESS_COUNT_MAX) {
		if (_process_list[current].metadata.state == Running) break;
		++current;
	}

	memcpy(&_process_list[current].context.frame, frame, sizeof(struct InterruptFrame));

	int next = current + 1;
	while (true) {
		if (next == current) break;
		if (_process_list[next].metadata.state == Waiting) break;
		next = (next + 1) % PROCESS_COUNT_MAX;
	}

	// Only one process active, no need to switch
	if (current == next) return;

	memcpy(frame, &_process_list[next].context.frame, sizeof(struct InterruptFrame));

	return;
};

void scheduler_init(void) {
	activate_timer_interrupt();

	int i = 0;
	while (i < PROCESS_COUNT_MAX) {
		if (_process_list[i].metadata.state == Waiting) break;
		++i;
	}

	// Error, no init process
	if (i == PROCESS_COUNT_MAX)
		return;

	_process_list[i].metadata.state = Running;

	paging_use_page_directory(_process_list[i].context.page_directory_virtual_addr);

	// Assume start program always in 0
	kernel_execute_user_program((void *)0);
	// kernel_start_user_mode(&_process_list[i].context.frame);
};
