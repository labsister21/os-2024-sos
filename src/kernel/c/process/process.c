#include "process/process.h"
#include "cpu/gdt.h"
#include "filesystem/vfs.h"
#include "memory/paging.h"
#include <std/string.h>

struct {
	uint32_t active_process_count;
	uint32_t last_pid;
} process_manager_state = {
		.last_pid = 0,
		.active_process_count = 0,
};

struct ProcessControlBlock _process_list[PROCESS_COUNT_MAX];

int32_t process_generate_new_pid() {
	return process_manager_state.last_pid++;
};

int32_t process_list_get_inactive_index() {
	for (int i = 0; i < PROCESS_COUNT_MAX; ++i) {
		if (_process_list[i].metadata.state == Inactive) return i;
	}
	return -1;
}

int32_t process_create_user_process(char *path) {
	// Path needs to be copied, since we will be changing page directory
	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);
	path = copy;

	int32_t retcode = PROCESS_CREATE_SUCCESS;
	if (process_manager_state.active_process_count >= PROCESS_COUNT_MAX) {
		retcode = PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED;
		goto exit_cleanup;
	}

	struct VFSEntry entry;
	fat32_vfs.stat(path, &entry);

	// Check whether memory is enough for the executable and additional frame for user stack
	uint32_t page_frame_count_needed = (entry.size + PAGE_FRAME_SIZE + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;
	if (!paging_allocate_check(page_frame_count_needed) || page_frame_count_needed > PROCESS_PAGE_FRAME_COUNT_MAX) {
		retcode = PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY;
		goto exit_cleanup;
	}

	// Process PCB
	int32_t p_index = process_list_get_inactive_index();
	struct ProcessControlBlock *pcb = &(_process_list[p_index]);
	pcb->metadata.pid = process_generate_new_pid();
	pcb->metadata.state = Waiting;

	// Creating page directory
	struct PageDirectory *page_directory = paging_create_new_page_directory();
	pcb->context.page_directory_virtual_addr = page_directory;

	struct PageDirectory *current_page_directory = paging_get_current_page_directory_addr();
	paging_use_page_directory(page_directory);

	void *program_base_address = 0;
	paging_allocate_user_page_frame(page_directory, program_base_address);

	int fd = fat32_vfs.open(path);
	fat32_vfs.read(fd, program_base_address, entry.size);
	fat32_vfs.close(fd);

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
exit_cleanup:
	return retcode;
}
