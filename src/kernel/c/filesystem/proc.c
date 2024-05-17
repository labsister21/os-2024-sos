#include "filesystem/proc.h"
#include "filesystem/vfs.h"
#include "memory/kmalloc.h"
#include "process/process.h"
#include "text/framebuffer.h"
#include <std/string.h>

static int status;
static int parse_path(char *path, bool *is_root) {
	*is_root = false;
	if (strcmp(path, "/proc") == 0) {
		*is_root = true;
		return 0;
	}

	strtok(path, '/');
	char *pid = strtok(NULL, '/');
	char *trail = strtok(NULL, '/');

	if (trail != NULL)
		return -1;

	int result = strtoi(pid, NULL);
	return result;
}

static struct ProcessControlBlock *get_pcb_from_pid(int pid) {
	int idx = 0;
	while (idx < PROCESS_COUNT_MAX) {
		struct ProcessControlBlock *current = &_process_list[idx];
		if (current->metadata.state == Inactive) goto end_loop;
		if (current->metadata.pid == pid) return current;

	end_loop:
		idx += 1;
	}
	return NULL;
}

static int process_stat(int pid, struct VFSEntry *entry) {
	struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);
	if (pcb == NULL)
		return -1;

	itoa(pid, entry->name, 10);
	entry->size = 0;
	entry->type = File;

	return 0;
}

static int count_running_process() {
	int count = 0;
	for (int i = 0; i < PROCESS_COUNT_MAX; ++i) {
		if (_process_list[i].metadata.state == Inactive) continue;
		count += 1;
	}
	return count;
}

static int stat(char *path, struct VFSEntry *entry) {
	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);

	bool is_root;
	int pid = parse_path(copy, &is_root);
	if (pid < 0)
		return pid;

	if (is_root) {
		strcpy(entry->name, "proc", 255);
		entry->size = count_running_process();
		entry->type = Directory;
		return 0;
	}

	status = process_stat(pid, entry);
	return status;
};

static int dirstat(char *path, struct VFSEntry *entries) {
	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);

	bool is_root;
	int pid = parse_path(copy, &is_root);
	if (pid < 0)
		return pid;

	if (!is_root)
		return -1;

	int count = 0;
	for (int i = 0; i < PROCESS_COUNT_MAX; ++i) {
		struct ProcessControlBlock *current = &_process_list[i];
		if (current->metadata.state == Inactive) continue;
		process_stat(current->metadata.pid, &entries[count++]);
	}

	return 0;
};

struct VFSState {
	struct VFSFileTableEntry entry;
	struct ProcessControlBlock *pcb;
	int current_pointer;
	int max_pointer;
};

static int open(char *path) {
	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);

	bool is_root;
	int pid = parse_path(copy, &is_root);
	if (pid < 0)
		return pid;

	if (is_root)
		return -1;

	struct VFSState *state = kmalloc(sizeof(struct VFSState));
	state->entry.handler = &proc_vfs;

	int ft = register_file_table((void *)state);
	if (ft < 0) {
		kfree(state);
		return -1;
	}

#define MAX_DIGIT 16

	struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);

	state->pcb = pcb;
	state->current_pointer = 0;
	state->max_pointer = str_len(pcb->metadata.name);

	// char digits[MAX_DIGIT];
	// strcat(state->buffer, "EIP: 0x", 1000);
	// itoa(pcb->context.frame.int_stack.eip, digits, 16);
	// strcat(state->buffer, digits, 1000);
	// state->max_pointer = 100;

	return ft;
};

static int close(int ft) {
	(void)ft;
	return -1;
};

static int read(int ft, char *buffer, int size) {
	struct VFSState *state = (void *)get_vfs_table_entry(ft);

	if (state->current_pointer == state->max_pointer)
		return -1;

	int read_count = 0;
	while (true) {
		if (read_count >= size) break;
		if (state->current_pointer == state->max_pointer) break;
		buffer[read_count++] = state->pcb->metadata.name[state->current_pointer++];
	}

	return read_count;
};

struct VFSHandler proc_vfs = {
		.stat = stat,
		.dirstat = dirstat,

		.open = open,
		.close = close,

		.read = read,
		.write = NULL,

		.mkfile = NULL,
		.mkdir = NULL,

		.delete = NULL,
};
