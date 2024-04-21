#include <fat32.h>
#include <std/stdint.h>
#include <std/string.h>
#include <syscall.h>

#define MAX_PROMPT 512

struct ShellState {
	struct FAT32DirectoryTable curr_dir;
	char prompt[MAX_PROMPT];
	int prompt_size;
};
struct ShellState state = {};

void clear() {
	syscall_FRAMEBUFFER_CLEAR();
}

void refresh_curr_dir() {
	struct FAT32DriverRequest req;
	req.parent_cluster_number = get_cluster_from_dir_entry(&state.curr_dir.table[0]);
	req.buffer_size = CLUSTER_SIZE;
	req.buf = &state.curr_dir;
	strcpy(req.name, ".", 8);
	int8_t ret;
	syscall_READ_DIRECTORY(&req, &ret);
}

void ls() {
	for (int i = 0; i < MAX_DIR_TABLE_ENTRY; ++i) {
		struct FAT32DirectoryEntry *entry = &state.curr_dir.table[i];
		if (entry->user_attribute != UATTR_NOT_EMPTY) continue;
		syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(entry->name);
		if (entry->attribute != ATTR_SUBDIRECTORY) syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(entry->ext);
		syscall_FRAMEBUFFER_PUT_CHAR(' ');
	}
}

void cd() {
	struct FAT32DriverRequest req;
	char *dir;
	dir = strtok(NULL, '\0');
	strcpy(req.name, dir, 8);
	req.parent_cluster_number = get_cluster_from_dir_entry(&state.curr_dir.table[0]);
	req.buf = &state.curr_dir;
	req.buffer_size = CLUSTER_SIZE;

	int8_t ret;
	syscall_READ_DIRECTORY(&req, &ret);

	if (ret == 0) {
		if (strcmp(dir, ".") == 0) {
			syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("stay in directory");
		} else if (strcmp(dir, "..") == 0) {
			syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("moved to parent folder");
		} else {
			syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("moved to ");
			syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(dir);
		}
	}
	if (ret == 2) {
		syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("directory not found");
	} else if (ret == 1) {
		syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("can't open file as directory");
	}
}

void mkdir() {
	struct FAT32DriverRequest req;
	char *dir;
	dir = strtok(NULL, '\0');
	strcpy(req.name, dir, 8);
	req.buf = NULL;
	req.buffer_size = 0;
	req.parent_cluster_number = get_cluster_from_dir_entry(&state.curr_dir.table[0]);
	int8_t ret;
	syscall_WRITE(&req, &ret);

	syscall_FRAMEBUFFER_PUT_CHAR(ret + '0');
	refresh_curr_dir();
}

void get_prompt() {
	state.prompt_size = 0;

	char c;
	while (1) {
		syscall_GET_CHAR(&c);
		if (c == '\n' || state.prompt_size + 1 >= MAX_PROMPT) break;
		state.prompt[state.prompt_size++] = c;
	}
	state.prompt[state.prompt_size] = '\0';
}

void run_prompt() {
	char *token = strtok(state.prompt, ' ');
	if (strcmp(token, "ls") == 0) ls();
	else if (strcmp(token, "cd") == 0) cd();
	else if (strcmp(token, "mkdir") == 0) mkdir();
	else if (strcmp(token, "clear") == 0) clear();
	else {
		char *not_found = "command not found!";
		syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(not_found);
	}
	syscall_PUT_CHAR('\n');
}

int main(void) {
	int8_t ret;

	struct FAT32DriverRequest req;
	req.parent_cluster_number = ROOT_CLUSTER_NUMBER;
	req.buffer_size = 0;
	strcpy(req.name, "dir", 8);
	syscall_WRITE(&req, &ret);

	req.parent_cluster_number = ROOT_CLUSTER_NUMBER;
	req.buf = &state.curr_dir;
	req.buffer_size = sizeof(struct FAT32DirectoryTable);
	strcpy(req.name, ".", 8);
	syscall_READ_DIRECTORY(&req, &ret);

	while (true) {
		syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS("> ");
		get_prompt();
		run_prompt();
	}
	return 0;
}
