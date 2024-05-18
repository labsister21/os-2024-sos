#include "process/scheduler.h"
#include "cpu/portio.h"
#include "memory/kmalloc.h"
#include "process/process.h"
#include "text/framebuffer.h"
#include <std/stdint.h>
#include <std/string.h>

extern void kernel_start_user_mode(void *);

struct LinkedPCB {
	struct ProcessControlBlock *pcb;
	struct LinkedPCB *next;
	struct LinkedPCB *prev;
};

struct LinkedPCB *current_running = NULL;
struct LinkedPCB *queue_front = NULL;
struct LinkedPCB *queue_back = NULL;

void scheduler_add(struct ProcessControlBlock *pcb) {
	struct LinkedPCB *node = kmalloc(sizeof(struct LinkedPCB));
	node->pcb = pcb;
	node->prev = queue_back;
	node->next = NULL;

	if (queue_back != NULL)
		queue_back->next = node;
	queue_back = node;

	if (queue_front == NULL)
		queue_front = node;
}

void scheduler_remove(struct ProcessControlBlock *pcb) {
	if (current_running->pcb == pcb) return;

	struct LinkedPCB *current = queue_front;
	while (current != NULL) {
		if (current->pcb == pcb) {
			if (current->prev)
				current->prev->next = current->next;

			if (current->next)
				current->next->prev = current->prev;

			if (current == queue_front)
				queue_front = current->next;
			if (current == queue_back)
				queue_back = current->prev;

			kfree(current);
			break;
		}

		current = current->next;
	}
}

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

bool context_switch() {
	if (queue_front == NULL && queue_back == NULL) return false;

	// Put current running to queue_back
	current_running->prev = queue_back;
	current_running->next = NULL;
	queue_back->next = current_running;
	queue_back = current_running;

	// Change current running to queue_front
	current_running = queue_front;
	queue_front = current_running->next;
	if (queue_front)
		queue_front->prev = NULL;
	if (current_running == queue_back)
		queue_back = current_running->next;
	current_running->next = NULL;

	return true;
}

void scheduler_handle_timer_interrupt(struct InterruptFrame *frame) {
	pic_ack(PIC1_OFFSET + IRQ_TIMER);

	struct ProcessControlBlock *prev_pcb = current_running->pcb;
	memcpy(&prev_pcb->context.frame, frame, sizeof(struct InterruptFrame));
	prev_pcb->metadata.state = Waiting;

	context_switch();

	struct ProcessControlBlock *next_pcb = current_running->pcb;
	paging_use_page_directory(next_pcb->context.memory.page_directory_virtual_addr);
	memcpy(frame, &next_pcb->context.frame, sizeof(struct InterruptFrame));
	next_pcb->metadata.state = Running;

	return;
};

void scheduler_init(void) {
	if (current_running != NULL) return;

	activate_timer_interrupt();

	current_running = queue_front;
	queue_front = queue_front->next;
	if (queue_front)
		queue_front->prev = NULL;
	if (queue_back == current_running)
		queue_back = NULL;
	current_running->next = NULL;

	paging_use_page_directory(current_running->pcb->context.memory.page_directory_virtual_addr);
	kernel_start_user_mode(&current_running->pcb->context.frame);
};

int get_current_running_pid() {
	if (current_running == NULL) return -1;
	return current_running->pcb->metadata.pid;
};
