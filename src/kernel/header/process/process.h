#ifndef _PROCESS_H
#define _PROCESS_H

#include <std/stdbool.h>
#include <std/stddef.h>
#include <std/stdint.h>
#include <vfs.h>

#include "cpu/interrupt.h"
#include "memory/paging.h"

#define PROCESS_NAME_LENGTH_MAX 32
#define PROCESS_PAGE_FRAME_COUNT_MAX 8

#define PROCESS_COUNT_MAX 32
#define PROCESS_START_PID 1
#define PROCESS_END_PID (PROCESS_COUNT_MAX + PROCESS_START_PID)

#define KERNEL_RESERVED_PAGE_FRAME_COUNT 4
#define KERNEL_VIRTUAL_ADDRESS_BASE 0xC0000000

#define CPU_EFLAGS_BASE_FLAG 0x2
#define CPU_EFLAGS_FLAG_CARRY 0x1
#define CPU_EFLAGS_FLAG_PARITY 0x4
#define CPU_EFLAGS_FLAG_AUX_CARRY 0x10
#define CPU_EFLAGS_FLAG_ZERO 0x40
#define CPU_EFLAGS_FLAG_SIGN 0x80
#define CPU_EFLAGS_FLAG_TRAP 0x100
#define CPU_EFLAGS_FLAG_INTERRUPT_ENABLE 0x200
#define CPU_EFLAGS_FLAG_DIRECTION 0x400
#define CPU_EFLAGS_FLAG_OVERFLOW 0x800
#define CPU_EFLAGS_FLAG_IO_PRIVILEGE 0x3000
#define CPU_EFLAGS_FLAG_NESTED_TASK 0x4000
#define CPU_EFLAGS_FLAG_MODE 0x8000
#define CPU_EFLAGS_FLAG_RESUME 0x10000
#define CPU_EFLAGS_FLAG_VIRTUAL_8086 0x20000
#define CPU_EFLAGS_FLAG_ALIGNMENT_CHECK 0x40000
#define CPU_EFLAGS_FLAG_VINTERRUPT_FLAG 0x80000
#define CPU_EFLAGS_FLAG_VINTERRUPT_PENDING 0x100000
#define CPU_EFLAGS_FLAG_CPUID_INSTRUCTION 0x200000
#define CPU_EFLAGS_FLAG_AES_SCHEDULE_LOAD 0x40000000
#define CPU_EFLAGS_FLAG_ALTER_INSTRUCTION 0x80000000

// Return code constant for process_create_user_process()
#define PROCESS_CREATE_SUCCESS 0
#define PROCESS_CREATE_FAIL_MAX_PROCESS_EXCEEDED 1
#define PROCESS_CREATE_FAIL_INVALID_ENTRYPOINT 2
#define PROCESS_CREATE_FAIL_NOT_ENOUGH_MEMORY 3
#define PROCESS_CREATE_FAIL_FS_READ_FAILURE 4

#define PROCESS_MAX_FD 16

enum ProcessState {
	Inactive,
	Running,
	Ready,
	Waiting
};

/**
 * Contain information needed for task to be able to get interrupted and resumed later
 *
 * @param cpu                         All CPU register state
 * @param eip                         CPU instruction counter to resume execution
 * @param eflags                      Flag register to load before resuming the execution
 * @param page_directory_virtual_addr CPU register CR3, containing pointer to active page directory
 */
struct ProcessContext {
	struct InterruptFrame frame;
	struct {
		uint32_t page_frame_used_count;
		struct PageDirectory *page_directory_virtual_addr;
		void *virtual_addr_used[PROCESS_PAGE_FRAME_COUNT_MAX];
	} memory;
};

struct ProcessNotifier {
	void *closure;
	bool (*predicate)(void *closure);
	bool recall;
};

/**
 * Structure data containing information about a process
 *
 * @param metadata Process metadata, contain various information about process
 * @param context  Process context used for context saving & switching
 * @param memory   Memory used for the process
 */
struct ProcessControlBlock {
	struct ProcessMetadata {
		int pid;
		enum ProcessState state;
		char name[MAX_VFS_NAME];
	} metadata;

	struct ProcessContext context;
	struct ProcessNotifier notifier;

	int fd[PROCESS_MAX_FD]; // File descriptor table
};

/**
 * Create new user process and setup the virtual address space.
 * All available return code is defined with macro "PROCESS_CREATE_*"
 *
 * @note          This procedure assumes no reentrancy in ISR
 * @warning       Assuming read(request) always success and request.buf point to load address
 * @param request Appropriate read request for the executable
 * @return        Process creation return code
 */
int process_create(char *path);

/**
 * Destroy process then release page directory and process control block
 *
 * @param pid Process ID to delete
 * @return    True if process destruction success
 */
int process_destroy(int pid);

struct ProcessControlBlock *get_pcb_from_pid(int pid);

void process_current_sleep(uint32_t seconds);

#endif
