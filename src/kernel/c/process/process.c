#include "process/process.h"
#include "cpu/gdt.h"
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

// TODO: Change parameter signature to path only
int32_t process_create_user_process(struct FAT32DriverRequest *request) {
	int32_t retcode = PROCESS_CREATE_SUCCESS;
	if (process_manager_state.active_process_count >= PROCESS_COUNT_MAX) {
		retcode = PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED;
		goto exit_cleanup;
	}

	// Ensure entrypoint is not located at kernel's section at higher half
	if ((uint32_t)request->buf >= KERNEL_VIRTUAL_ADDRESS_BASE) {
		retcode = PROCESS_CREATE_FAIL_INVALID_ENTRYPOINT;
		goto exit_cleanup;
	}

	// Check whether memory is enough for the executable and additional frame for user stack
	uint32_t page_frame_count_needed = (request->buffer_size + PAGE_FRAME_SIZE + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;
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
	pcb->context.frame.int_stack.eflags |= CPU_EFLAGS_BASE_FLAG | CPU_EFLAGS_FLAG_INTERRUPT_ENABLE;

	struct PageDirectory *current_page_directory = paging_get_current_page_directory_addr();
	paging_use_page_directory(page_directory);
	void *program_base_address = request->buf;
	paging_allocate_user_page_frame(page_directory, program_base_address);
	read(request);
	paging_use_page_directory(current_page_directory);

	pcb->memory.virtual_addr_used[0] = program_base_address;
	pcb->memory.page_frame_used_count += 1;

	process_manager_state.active_process_count += 1;
exit_cleanup:
	return retcode;
}
