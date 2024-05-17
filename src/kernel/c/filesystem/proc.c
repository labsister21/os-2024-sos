#include "filesystem/proc.h"
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

	itoa(pid, entry->name);
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
	bool is_root;
	int pid = parse_path(path, &is_root);
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
	bool is_root;
	int pid = parse_path(path, &is_root);
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

static int open(char *path) {
	(void)path;
	return -1;
};

static int close(int ft) {
	(void)ft;
	return -1;
};

static int read(int ft, char *buffer, int size) {
	(void)ft;
	(void)buffer;
	(void)size;
	return -1;
};

static int write(int ft, char *buffer, int size) {
	(void)ft;
	(void)buffer;
	(void)size;
	return -1;
};

static int mkfile(char *path) {
	(void)path;
	return -1;
};

static int mkdir(char *path) {
	(void)path;
	return -1;
};

static int delete(char *path) {
	(void)path;
	return -1;
};

struct VFSHandler proc_vfs = {
		.stat = stat,
		.dirstat = dirstat,

		.open = open,
		.close = close,

		.read = read,
		.write = write,

		.mkfile = mkfile,
		.mkdir = mkdir,

		.delete = delete,
};
