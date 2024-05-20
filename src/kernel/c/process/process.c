#include "process/process.h"
#include "filesystem/vfs.h"
#include "memory/kmalloc.h"
#include "memory/memory.h"
#include "memory/paging.h"
#include "process/file_descriptor.h"
#include "process/scheduler.h"
#include <path.h>
#include <std/string.h>
#include <vfs.h>

struct {
	uint32_t active_process_count;
} process_manager_state = {
		.active_process_count = 0,
};

struct ProcessControlBlock *process_pids[PROCESS_COUNT_MAX];
static int get_free_pid() {
	int idx = 0;
	while (idx < PROCESS_COUNT_MAX) {
		if (process_pids[idx] == NULL) break;
		idx += 1;
	}
	if (idx == PROCESS_COUNT_MAX)
		return -1;
	return idx + PROCESS_START_PID;
}

static void set_free_pid(int pid) {
	int idx = pid - PROCESS_START_PID;
	process_pids[idx] = NULL;
}

static void reserve_pid(int pid, struct ProcessControlBlock *pcb) {
	int idx = pid - PROCESS_START_PID;
	process_pids[idx] = pcb;
	pcb->metadata.pid = pid;
}

struct ProcessControlBlock *get_pcb_from_pid(int pid) {
	return process_pids[pid - PROCESS_START_PID];
}

void setup_register(struct ProcessControlBlock *pcb) {
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
}

int process_create(char *p) {
	// Path needs to be copied, since we will be changing page directory
	COPY_STRING_TO_LOCAL(path, p);

	struct ProcessControlBlock *pcb = kmalloc(sizeof(struct ProcessControlBlock));
	struct PageDirectory *page_directory = paging_create_new_page_directory();
	if (pcb == NULL || page_directory == NULL)
		goto error;
	memset(pcb, 0, sizeof(struct ProcessControlBlock));

	if (process_manager_state.active_process_count >= PROCESS_COUNT_MAX)
		goto error;

	struct VFSEntry entry;
	int status = vfs.stat(path, &entry);
	if (status != 0 || entry.type != File)
		goto error;

	// Check whether memory is enough for the executable and additional frame for user stack
	uint32_t page_frame_count_needed = (entry.size + PAGE_FRAME_SIZE + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;
	if (!paging_allocate_check(page_frame_count_needed) || page_frame_count_needed > PROCESS_PAGE_FRAME_COUNT_MAX) goto error;

	int pid = get_free_pid();
	if (pid < 0) goto error;
	pcb->metadata.pid = pid;
	pcb->metadata.state = Ready;

	for (int i = 0; i < PROCESS_MAX_FD; ++i)
		pcb->fd[i] = -1;

	struct PageDirectory *current_page_directory = paging_get_current_page_directory_addr();
	void *program_base_address = 0;
	status = paging_allocate_user_page_frame(page_directory, program_base_address);
	if (!status) goto error;

	paging_use_page_directory(page_directory);
	int ft = vfs.open(path);
	if (ft < 0)
		goto error;
	vfs.read(ft, program_base_address, entry.size);
	vfs.close(ft);
	paging_use_page_directory(current_page_directory);

	setup_register(pcb);
	pcb->context.memory.page_directory_virtual_addr = page_directory;
	pcb->context.memory.virtual_addr_used[0] = program_base_address;
	pcb->context.memory.page_frame_used_count += 1;

	process_manager_state.active_process_count += 1;

	char *basename;
	split_path(path, NULL, &basename);
	strcpy(pcb->metadata.name, basename, MAX_VFS_NAME);

	reserve_pid(pid, pcb);
	scheduler_add(pcb);
	return pid;

error:
	if (pcb != NULL) kfree(pcb);
	if (page_directory != NULL) paging_free_page_directory(page_directory);
	return -1;
}

int process_destroy(int pid) {
	if (pid == get_current_running_pid()) return -1;

	struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);
	pcb->metadata.state = Inactive;
	cleanup_fd(pcb);
	scheduler_remove(pcb);
	paging_free_page_directory(pcb->context.memory.page_directory_virtual_addr);
	kfree(pcb);
	set_free_pid(pid);

	process_manager_state.active_process_count -= 1;
	return 0;
};
