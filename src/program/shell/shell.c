#include <fat32.h>
#include <std/stdint.h>
#include <std/string.h>
#include <syscall.h>

#define MAX_PROMPT 512

struct ShellState {
	struct FAT32DirectoryTable curr_dir;
	char prompt[MAX_PROMPT];
	int prompt_size;
	int cursor_y;
	int cursor_x;
};
struct ShellState state = {.cursor_y = 0, .cursor_x = 0};

void clear() {
	state.cursor_x = 0;
	state.cursor_y = 0;
	syscall_FRAMEBUFFER_CLEAR();
}

void next_line() {
	state.cursor_x = 0;
	state.cursor_y += 1;
	if (state.cursor_y > 25) clear();
	syscall_FRAMEBUFFER_CURSOR(state.cursor_x, state.cursor_y);
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
	dir = strtok(NULL, ' ');
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

void get_prompt() {
	state.prompt_size = 0;
	while (state.prompt_size + 1 < MAX_PROMPT) {
		char n;
		syscall(0x4, (uint32_t)&n, 0, 0);
		if (n == '\n') break;
		if (n == '\b') {
			if (state.cursor_x == 0) continue;
			n = ' ';
			state.cursor_x -= 1;
			syscall_FRAMEBUFFER_CURSOR(state.cursor_x + 2, state.cursor_y);
			syscall_FRAMEBUFFER_PUT_CHAR(n);
			syscall_FRAMEBUFFER_CURSOR(state.cursor_x + 2, state.cursor_y);
			state.prompt_size -= 1;
			continue;
		}
		syscall_FRAMEBUFFER_PUT_CHAR(n);
		state.prompt[state.prompt_size++] = n;
		state.cursor_x += 1;
	};
	state.prompt[state.prompt_size] = '\0';
	next_line();
}

void run_prompt() {
	char *token = strtok(state.prompt, ' ');
	if (strcmp(token, "ls") == 0) ls();
	else if (strcmp(token, "cd") == 0) cd();
	else if (strcmp(token, "clear") == 0) clear();
	else {
		char *not_found = "command not found!";
		syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(not_found);
	}
	next_line();
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
