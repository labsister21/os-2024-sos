#include <fat32.h>
#include <std/stdint.h>
#include <std/string.h>
#include <syscall.h>

#define MAX_PROMPT 512
#define MAX_PATH 1024
#define MAX_ENTRIES 128

int status;
struct ShellState {
	char cwd_path[MAX_PATH];
	struct VFSEntry cwd_entry;
	struct VFSEntry cwd_entries[MAX_ENTRIES]; // Waiting for malloc
	struct FAT32DirectoryTable curr_dir;
	char prompt[MAX_PROMPT];
	int prompt_size;
};
struct ShellState state = {};

void puts(char *str) {
	syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(str);
}

int refresh_cwd() {
	status = syscall_VFS_STAT(state.cwd_path, &state.cwd_entry);
	if (status != 0) return status;
	status = syscall_VFS_DIR_STAT(state.cwd_path, state.cwd_entries);
	if (status != 0) return status;

	return 0;
}

// Botched
void join_path(char *result, char *base, char *next) {
	strcpy(result, base, MAX_PATH);
	if (strcmp(base, "/") != 0)
		strcat(result, "/", MAX_PATH);
	strcat(result, next, MAX_PATH);
}

void clear() {
	syscall_PUT_CHAR('\e');
	syscall_PUT_CHAR('J');
}

void ls() {
	for (int i = 0; i < state.cwd_entry.size; ++i) {
		puts(state.cwd_entries[i].name);
		puts(" ");
	}
}

void cd() {
	char *next = strtok(NULL, ' ');

	if (strcmp(next, ".") == 0) {
		return;
	} else if (strcmp(next, "..") == 0) {
		int size = str_len(state.cwd_path);

		int i = size - 1;
		while (i >= 0) {
			if (state.cwd_path[i] == '/') break;
			i -= 1;
		}

		if (i == 0)
			i += 1;
		state.cwd_path[i] = '\0';
		puts("Directory changed");
	} else {
		int cwd_size = str_len(state.cwd_path);
		int size = cwd_size + str_len(next) + 2;
		if (size > MAX_PATH) size = MAX_PATH;

		char next_path[size];
		next_path[0] = '\0';
		strcpy(next_path, state.cwd_path, size);
		if (cwd_size > 1) // Non root directory
			strcat(next_path, "/", size);
		strcat(next_path, next, size);

		struct VFSEntry next_entry;
		status = syscall_VFS_STAT(next_path, &next_entry);
		if (status != 0) {
			puts("Error reading directory");
			return;
		}

		if (next_entry.type == File) {
			puts("Can't move to file as directory");
			return;
		}

		puts("Directory changed");
		strcpy(state.cwd_path, next_path, size);
	}
}

void mkdir() {
	char *next = strtok(NULL, ' ');
	status = syscall_VFS_MKDIR(state.cwd_path, next);
	if (status != 0)
		puts("Error creating directory");
	else
		puts("Directory created");
}

void tac() {
	char *filename = strtok(NULL, ' ');

	status = syscall_VFS_MKFILE(state.cwd_path, filename);
	if (status != 0) {
		puts("Error creating file");
		return;
	}

	char fullpath[MAX_PATH];
	join_path(fullpath, state.cwd_path, filename);

	int fd = syscall_VFS_OPEN(fullpath);
	if (fd < 0) {
		puts("Error opening file");
		return;
	}

	char *content = strtok(NULL, '\0');
	syscall_VFS_WRITE(fd, content, str_len(content));
	syscall_VFS_WRITE(fd, "\0", 1);
}

void cat() {
	char *filename = strtok(NULL, ' ');

	char fullpath[MAX_PATH];
	join_path(fullpath, state.cwd_path, filename);

	struct VFSEntry entry;
	status = syscall_VFS_STAT(fullpath, &entry);
	if (status != 0) {
		puts("Error getting file stat");
		return;
	}

	int fd = syscall_VFS_OPEN(fullpath);
	if (fd < 0) {
		puts("Error opening file");
		return;
	}

	int i = 0;
	int block = 512;
	while (i < entry.size) {
		char buff[block];
		syscall_VFS_READ(fd, buff, block);
		puts(buff);

		i += block;
	}
}

void cp() {
	char *from = strtok(NULL, ' ');
	char *to = strtok(NULL, ' ');

	char fullpath_from[MAX_PATH];
	join_path(fullpath_from, state.cwd_path, from);

	struct VFSEntry from_entry;
	syscall_VFS_STAT(fullpath_from, &from_entry);
	if (from_entry.type == Directory) {
		puts("Can't copy directory");
		return;
	}

	int from_fd = syscall_VFS_OPEN(fullpath_from);
	if (from_fd < 0) {
		puts("Error opening source");
		return;
	}

	char fullpath_to[MAX_PATH];
	join_path(fullpath_to, state.cwd_path, to);

	int status = syscall_VFS_MKFILE(state.cwd_path, to);
	if (status != 0) {
		puts("Error creating file");
		return;
	}

	int to_fd = syscall_VFS_OPEN(fullpath_to);
	if (to_fd < 0) {
		puts("Error opening new created file");
		return;
	}

	int block = 512;
	char buffer[block];
	while (syscall_VFS_READ(from_fd, buffer, block) > 0) {
		syscall_VFS_WRITE(to_fd, buffer, block);
	}

	puts("Copy succeed");
}

void exec() {
	int pid = syscall_EXEC(strtok(NULL, ' '));
	syscall_PUT_CHAR('0' + pid);
}

void kill() {
	char *token = strtok(NULL, ' ');
	int pid = strtoi(token, NULL);
	if (pid == -1) {
		puts("Invalid PID");
		return;
	}

	status = syscall_KILL(pid);
	if (status != 0) {
		puts("Error killing process");
		return;
	}

	puts("Process killed");
}

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
	else if (strcmp(token, "mkdir") == 0) mkdir();
	else if (strcmp(token, "cd") == 0) cd();
	else if (strcmp(token, "cat") == 0) cat();
	else if (strcmp(token, "tac") == 0) tac();
	else if (strcmp(token, "cp") == 0) cp();
	else if (strcmp(token, "exec") == 0) exec();
	else if (strcmp(token, "kill") == 0) kill();
	else {
		char *not_found = "command not found!";
		puts(not_found);
	}

	if (!isClear) syscall_PUT_CHAR('\n');
}

int main(void) {
	strcpy(state.cwd_path, "/", 2);
	status = refresh_cwd();
	if (status != 0) {
		puts("Error reading root directory");
		return status;
	}

	while (true) {
		puts(state.cwd_path);
		puts(" > ");
		get_prompt();
		run_prompt();
		refresh_cwd();
	}
	return 0;
}
