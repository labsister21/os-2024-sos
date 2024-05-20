#include "filesystem/proc.h"
#include "filesystem/vfs.h"
#include "memory/kmalloc.h"
#include "process/process.h"
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

static int process_stat(struct ProcessControlBlock *pcb, struct VFSEntry *entry) {
	if (pcb == NULL)
		return -1;

	itoa(pcb->metadata.pid, entry->name, 10);
	entry->size = 0;
	entry->type = File;

	return 0;
}

static int count_running_process() {
	int count = 0;
	for (int pid = PROCESS_START_PID; pid < PROCESS_END_PID; ++pid) {
		struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);
		if (pcb == NULL || pcb->metadata.state == Inactive) continue;
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

	status = process_stat(get_pcb_from_pid(pid), entry);
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
	for (int pid = PROCESS_START_PID; pid < PROCESS_END_PID; ++pid) {
		struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);
		if (pcb == NULL || pcb->metadata.state == Inactive) continue;
		process_stat(pcb, &entries[count++]);
	}

	return 0;
};

struct VFSState {
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

	int ft = register_file_table_context((void *)state);
	if (ft < 0) {
		kfree(state);
		return -1;
	}

	struct ProcessControlBlock *pcb = get_pcb_from_pid(pid);

	state->pcb = pcb;
	state->current_pointer = 0;
	state->max_pointer = str_len(pcb->metadata.name) + 1;

	return ft;
};

static int close(int ft) {
	(void)ft;
	return -1;
};

static int read(int ft, char *buffer, int size) {
	struct VFSState *state = (void *)get_file_table_context(ft);

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
