#include <fat32.h>
#include <path.h>
#include <std/stdint.h>
#include <std/string.h>
#include <syscall.h>

#define MAX_PROMPT 512
#define MAX_PATH 1024
#define MAX_ENTRIES 128

int status;
struct ShellState {
	char cwd_path[MAX_PATH];
	char prompt[MAX_PROMPT];
};
struct ShellState state = {};

void puts(char *str) {
	syscall_FRAMEBUFFER_PUT_NULL_TERMINATED_CHARS(str);
}

void put_number(int number) {
	if (number < 0) {
		syscall_PUT_CHAR('-');
		number *= -1;
	}

	if (number > 9) put_number(number / 10);
	syscall_PUT_CHAR('0' + (number % 10));
}

void combine_path(char *result, char *base, char *next) {
	if (next[0] == '/') {
		strcpy(result, next, MAX_PATH);
		return;
	}

	strcpy(result, base, MAX_PATH);
	if (next == NULL) return;
	strcat(result, "/", MAX_PATH);
	strcat(result, next, MAX_PATH);
}

void clear() {
	syscall_PUT_CHAR('\e');
	syscall_PUT_CHAR('J');
}

void ls() {
	char *path = strtok(NULL, ' ');
	char fullpath[MAX_PATH];
	combine_path(fullpath, state.cwd_path, path);
	resolve_path(fullpath);

	struct VFSEntry entry;
	status = syscall_VFS_STAT(fullpath, &entry);
	if (status != 0) {
		puts("Error reading stat");
		return;
	}

	if (entry.type == File) {
		puts("Can't list file as directory");
		return;
	}

	struct VFSEntry entries[entry.size];
	status = syscall_VFS_DIR_STAT(fullpath, entries);
	if (status != 0) {
		puts("Error reading entries");
		return;
	}

	for (int i = 0; i < entry.size; ++i) {
		puts(entries[i].name);
		puts(" ");
	}
}

void cd() {
	char *next = strtok(NULL, ' ');
	char fullpath[MAX_PATH];
	if (next == NULL)
		next = "/";
	combine_path(fullpath, state.cwd_path, next);
	resolve_path(fullpath);

	struct VFSEntry next_entry;
	status = syscall_VFS_STAT(fullpath, &next_entry);
	if (status != 0) {
		puts("Error reading directory");
		return;
	}

	if (next_entry.type == File) {
		puts("Can't move to file as directory");
		return;
	}

	puts("Directory changed");
	strcpy(state.cwd_path, fullpath, MAX_PATH);
}

void stat() {
	char *path = strtok(NULL, ' ');
	char fullpath[MAX_PATH];
	combine_path(fullpath, state.cwd_path, path);
	resolve_path(fullpath);

	struct VFSEntry entry;
	status = syscall_VFS_STAT(fullpath, &entry);
	if (status != 0) {
		puts("Error reading stat");
		return;
	}

	puts("Name: ");
	puts(entry.name);
	syscall_PUT_CHAR('\n');
	puts("Type: ");
	puts(entry.type == File ? "File" : "Directory");
	syscall_PUT_CHAR('\n');
	puts("Size: ");
	put_number(entry.size);
}

void mkdir() {
	char *dir = strtok(NULL, ' ');
	char fullpath[MAX_PATH];
	combine_path(fullpath, state.cwd_path, dir);
	resolve_path(fullpath);

	status = syscall_VFS_MKDIR(fullpath);
	if (status != 0)
		puts("Error creating directory");
	else
		puts("Directory created");
}

void tac() {
	char *filename = strtok(NULL, ' ');
	char *content = strtok(NULL, '\0');
	char fullpath[MAX_PATH];
	combine_path(fullpath, state.cwd_path, filename);
	resolve_path(fullpath);

	status = syscall_VFS_MKFILE(fullpath);
	if (status != 0) {
		puts("Error creating file");
		return;
	}

	int fd = syscall_VFS_OPEN(fullpath);
	if (fd < 0) {
		puts("Error opening file");
		return;
	}

	syscall_VFS_WRITE(fd, content, str_len(content));
	syscall_VFS_WRITE(fd, "\0", 1);
}

void cat() {
	char *filename = strtok(NULL, ' ');
	char fullpath[MAX_PATH];
	combine_path(fullpath, state.cwd_path, filename);
	resolve_path(fullpath);

	struct VFSEntry entry;
	status = syscall_VFS_STAT(fullpath, &entry);
	if (status != 0) {
		puts("Error getting file stat");
		return;
	}

	if (entry.type == Directory) {
		puts("Can't cat directory");
		return;
	}

	int fd = syscall_VFS_OPEN(fullpath);
	if (fd < 0) {
		puts("Error opening file");
		return;
	}

	int block = 512;
	char buff[block];
	syscall_VFS_READ(fd, buff, block);
	puts(buff);
}

void cp() {
	char *from = strtok(NULL, ' ');
	char *to = strtok(NULL, ' ');

	char fullpath_from[MAX_PATH];
	combine_path(fullpath_from, state.cwd_path, from);
	resolve_path(fullpath_from);

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
	combine_path(fullpath_to, state.cwd_path, to);
	resolve_path(fullpath_to);

	int status = syscall_VFS_MKFILE(fullpath_to);
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
	char *name = strtok(NULL, ' ');
	char fullpath[MAX_PATH];
	combine_path(fullpath, state.cwd_path, name);
	resolve_path(fullpath);

	int pid = syscall_EXEC(fullpath);
	if (pid < 0) {
		puts("Error creating process");
		return;
	}
	puts("Process created with pid ");
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

void find() {
	char *search = strtok(NULL, ' ');
	char fullpath[MAX_PATH];
	combine_path(fullpath, state.cwd_path, search);
	resolve_path(fullpath);

	char file_list[100][8];
	char temp[MAX_PATH];
	strcpy(file_list[0], "/", 8);

	int i = 0;
	while (i < len(file_list)) {
		strcpy(temp, file_list[i], 8);
		struct VFSEntry entry;
		status = syscall_VFS_STAT(file_list[i], &entry);
		if (status != 0) {
			puts("Error reading stat");
			puts(file_list[i]);
			return;
		}

		struct VFSEntry entries[entry.size];
		status = syscall_VFS_DIR_STAT(file_list[i], entries);
		if (status != 0) {
			puts("Error reading entries");
			return;
		}

		for (int j = 0; j < entry.size; ++j) {
			char temp_path[MAX_PATH];
			strcpy(temp_path, temp, MAX_PATH);
			if (strcmp(entries[j].name, search) == 0) {
				if (i != 0) {
					strcat(temp_path, "/", MAX_PATH);
				}
				strcat(temp_path, entries[j].name, MAX_PATH);
				puts(temp_path);
				return;
			}
			if (entries[j].type == Directory) {
				if (i != 0) {
					strcat(temp_path, "/", MAX_PATH);
				}
				strcat(temp_path, entries[j].name, 8);
				push(file_list, temp_path);
			}
		}
		i++;
	}
	puts("No files or directory found");
}



void get_prompt() {
	int count = 0;
	while (1) {
		char c = '\0';
		while (c == '\0')
			syscall_GET_CHAR_NON_BLOCKING(&c);
		if (c != '\b' || (c == '\b' && str_len(state.prompt) != 0))
		{
			syscall_PUT_CHAR(c);
		}
		
		if (c == '\n' || count + 1 >= MAX_PROMPT)
			break;
		if (c == '\b'){
			if (str_len(state.prompt) != 0)
			{
				state.prompt[--count] = '\0';
			}
		}
		else {
			state.prompt[count++] = c;
		}
	}
	state.prompt[count] = '\0';
}

void run_prompt() {
	char *token = strtok(state.prompt, ' ');

	bool isClear = strcmp(token, "clear") == 0;

	if (isClear) clear();
	else if (strcmp(token, "ls") == 0) ls();
	else if (strcmp(token, "mkdir") == 0) mkdir();
	else if (strcmp(token, "cd") == 0) cd();
	else if (strcmp(token, "stat") == 0) stat();
	else if (strcmp(token, "cat") == 0) cat();
	else if (strcmp(token, "tac") == 0) tac();
	else if (strcmp(token, "cp") == 0) cp();
	else if (strcmp(token, "exec") == 0) exec();
	else if (strcmp(token, "kill") == 0) kill();
	else if (strcmp(token, "find") == 0) find();
	else {
		char *not_found = "command not found!";
		puts(not_found);
	}

	if (!isClear) syscall_PUT_CHAR('\n');
	if (str_len(state.prompt) != 0) {
		for (int i = 0; i < str_len(state.prompt); i++)
		{
			state.prompt[i] = '\0';
		}
	}
}

int get_max_combined_path_length(char *base, char *next) {
	return str_len(base) + str_len(next) + 1;
}

int main(void) {
	strcpy(state.cwd_path, "/", 2);

	while (true) {
		puts(state.cwd_path);
		puts(" > ");
		get_prompt();
		run_prompt();
	}
	return 0;
}
