#include "process/process.h"
#include "cpu/gdt.h"
#include "filesystem/vfs.h"
#include "memory/paging.h"
#include <std/string.h>

struct {
	uint32_t active_process_count;
	uint32_t last_pid;
} process_manager_state = {
		.last_pid = 1,
		.active_process_count = 0,
};

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX];

int process_generate_new_pid() {
	return process_manager_state.last_pid++;
};

int process_list_get_inactive_index() {
	for (int i = 0; i < PROCESS_COUNT_MAX; ++i) {
		if (_process_list[i].metadata.state == Inactive) return i;
	}
	return -1;
}

int process_create(char *path) {
	// Path needs to be copied, since we will be changing page directory
	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);
	path = copy;

	int32_t retcode = 0;
	if (process_manager_state.active_process_count >= PROCESS_COUNT_MAX) {
		retcode = -1;
		goto exit_cleanup;
	}

	struct VFSEntry entry;
	int status = vfs.stat(path, &entry);
	if (status != 0) {
		retcode = -1;
		goto exit_cleanup;
	}

	// Check whether memory is enough for the executable and additional frame for user stack
	uint32_t page_frame_count_needed = (entry.size + PAGE_FRAME_SIZE + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;
	if (!paging_allocate_check(page_frame_count_needed) || page_frame_count_needed > PROCESS_PAGE_FRAME_COUNT_MAX) {
		retcode = -1;
		goto exit_cleanup;
	}

	// Process PCB
	int32_t p_index = process_list_get_inactive_index();
	if (p_index < 0) {
		retcode = -1;
		goto exit_cleanup;
	}

	struct ProcessControlBlock *pcb = &(_process_list[p_index]);
	int pid = process_generate_new_pid();
	pcb->metadata.pid = pid;
	pcb->metadata.state = Waiting;

	// Creating page directory
	struct PageDirectory *page_directory = paging_create_new_page_directory();
	pcb->context.page_directory_virtual_addr = page_directory;

	struct PageDirectory *current_page_directory = paging_get_current_page_directory_addr();
	paging_use_page_directory(page_directory);

	void *program_base_address = 0;
	paging_allocate_user_page_frame(page_directory, program_base_address);

	int ft = vfs.open(path);
	vfs.read(ft, program_base_address, entry.size);
	vfs.close(ft);

	paging_use_page_directory(current_page_directory);

	pcb->memory.virtual_addr_used[0] = program_base_address;
	pcb->memory.page_frame_used_count += 1;

	// Setup register
	struct InterruptFrame *frame = &(pcb->context.frame);
	frame->cpu.index.edi = 0;
	frame->cpu.index.esi = 0;

	frame->cpu.stack.ebp = 0;
	frame->cpu.stack.esp = 0;

	frame->cpu.general.eax = 0;
	frame->cpu.general.ebx = 0;
	frame->cpu.general.ecx = 0;
	frame->cpu.general.edx = 0;

	uint32_t segment = 0x20 | 0x3;
	frame->cpu.segment.ds = segment;
	frame->cpu.segment.es = segment;
	frame->cpu.segment.fs = segment;
	frame->cpu.segment.gs = segment;

	frame->int_stack.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;
	frame->int_stack.cs = 0x18 | 0x3;
	frame->int_stack.eip = 0; // Assume always start at 0
	frame->int_stack.error_code = 0;
	frame->int_stack.old_esp = 0x400000 - 4;
	frame->int_stack.ss = segment;

	frame->int_number = 0;

	process_manager_state.active_process_count += 1;
	retcode = pid;
exit_cleanup:
	return retcode;
}

int process_destroy(int pid) {
	int idx = -1;
	int running = -1;

	int i = 0;
	while (i < PROCESS_COUNT_MAX && (idx == -1 || running == -1)) {
		struct ProcessControlBlock *current = &_process_list[i];
		if (
				current->metadata.state != Inactive &&
				current->metadata.pid == pid
		) idx = i;
		if (current->metadata.state == Running) running = i;
		i += 1;
	}
	if (idx == -1) return -1;

	// Process can't kill itself
	if (idx == running) return -1;

	struct ProcessControlBlock *pcb = &_process_list[idx];
	paging_free_page_directory(pcb->context.page_directory_virtual_addr);
	pcb->metadata.state = Inactive;
	process_manager_state.active_process_count -= 1;

	return 0;
};
