#include <fat32.h>
#include <std/stdint.h>
#include <std/string.h>
#include <syscall.h>

#define MAX_PROMPT 512

int status;
struct ShellState {
	struct VFSEntry cwd;
	struct VFSEntry cwd_entries[128]; // Waiting for malloc
	struct FAT32DirectoryTable curr_dir;
	char prompt[MAX_PROMPT];
	int prompt_size;
};
struct ShellState state = {};

void puts(char *str) {
	syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(str);
}

void clear() {
	syscall_PUT_CHAR('\e');
	syscall_PUT_CHAR('J');
}

void ls() {
	for (int i = 0; i < state.cwd.size; ++i) {
		puts(state.cwd_entries[i].name);
		puts(" ");
	}
}

void cd() {
}

void mkdir() {}

void get_prompt() {
	state.prompt_size = 0;

	while (1) {
		char c = '\0';
		while (c == '\0')
			syscall_GET_CHAR_NON_BLOCKING(&c);

		syscall_PUT_CHAR(c);
		if (c == '\n' || state.prompt_size + 1 >= MAX_PROMPT)
			break;
		state.prompt[state.prompt_size++] = c;
	}
	state.prompt[state.prompt_size] = '\0';
}

void run_prompt() {
	char *token = strtok(state.prompt, ' ');

	bool isClear = strcmp(token, "clear") == 0;

	if (isClear) clear();
	else if (strcmp(token, "ls") == 0) ls();
	else if (strcmp(token, "cd") == 0) cd();
	else if (strcmp(token, "mkdir") == 0) mkdir();
	else if (strcmp(token, "exec") == 0) syscall_EXEC(strtok(NULL, ' '));
	else {
		char *not_found = "command not found!";
		puts(not_found);
	}

	if (!isClear) syscall_PUT_CHAR('\n');
}

int main(void) {
	status = syscall_VFS_STAT("/", &state.cwd);
	if (status != 0) {
		puts("Error, can't read root directory");
		return 0;
	}

	syscall_VFS_DIR_STAT("/", state.cwd_entries);
	if (status != 0) {
		puts("Error, can't read root directory");
		return 0;
	}

	while (true) {
		puts("> ");
		get_prompt();
		run_prompt();
	}
	return 0;
}
