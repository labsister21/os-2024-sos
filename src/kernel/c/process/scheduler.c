#include "process/scheduler.h"
#include "cpu/portio.h"
#include "kernel-entrypoint.h"
#include <std/stdint.h>

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
};
