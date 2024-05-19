#include "filesystem/dev.h"
#include "driver/tty.h"
#include "memory/kmalloc.h"
#include "memory/memory.h"
#include "process/scheduler.h"
#include "text/framebuffer.h"
#include <path.h>
#include <std/stddef.h>

struct DevFileHandler {
	char name[MAX_VFS_NAME];

	// File table replaced with context pointer
	void *(*open)();
	int (*close)(void *context);

	int (*read)(void *context, char *buffer, int size);
	int (*write)(void *context, char *buffer, int size);
};

/* stdout */
void *stdout_open() {
	return NULL;
};

int stdout_close(void *context) {
	(void)context;
	return 0;
}

int stdout_write(void *context, char *buffer, int size) {
	(void)context;
	for (int i = 0; i < size; ++i) {
		framebuffer_put(buffer[i]);
	}
	return size;
}

/* stdin */
struct ForegroundList {
	int pid;
	struct ForegroundList *prev;
};
struct ForegroundList *top_foreground = NULL;

#define STDIN_BUFFER_SIZE 2
static char stdin_buffer[STDIN_BUFFER_SIZE];
static int stdin_buffer_current = 0;
static int stdin_buffer_last = 0;

void fill_stdin_buffer(char c) {
	stdin_buffer[stdin_buffer_last] = c;
	stdin_buffer_last = (stdin_buffer_last + 1) % STDIN_BUFFER_SIZE;

	if (stdin_buffer_current == stdin_buffer_last)
		stdin_buffer_current = (stdin_buffer_current + 1) % STDIN_BUFFER_SIZE;

	fputc(c);
}

void *stdin_open() {
	int pid = get_current_running_pid();
	if (pid < 0)
		return (void *)-1;

	struct ForegroundList *current = top_foreground;
	while (current != NULL) {
		if (current->pid == pid)
			return (void *)-1;
		current = current->prev;
	}

	struct ForegroundList *new = kmalloc(sizeof(struct ForegroundList));
	new->prev = top_foreground;
	top_foreground = new;

	return new;
}

int stdin_close(void *c) {
	struct ForegroundList *context = c;

	struct ForegroundList *next = NULL;
	struct ForegroundList *current = top_foreground;
	while (current != NULL) {
		if (current == context) {
			if (context == top_foreground)
				top_foreground = context->prev;

			if (next)
				next->prev = context->prev;
			kfree(context);

			return 0;
		}
		next = current;
		current = current->prev;
	}

	return -1;
}

int stdin_read(void *c, char *buffer, int size) {
	struct ForegroundList *context = c;
	if (context != top_foreground) return 0;

	int read_count = 0;
	while (true) {
		if (stdin_buffer_current == stdin_buffer_last) break;
		if (read_count >= size) break;

		buffer[read_count++] = stdin_buffer[stdin_buffer_current];
		stdin_buffer_current = (stdin_buffer_current + 1) % STDIN_BUFFER_SIZE;
	}
	return read_count;
}

#define FILE_COUNT 2
struct DevFileHandler dev_file_handlers[FILE_COUNT] = {
		{.name = "stdin",
		 .open = stdin_open,
		 .close = stdin_close,
		 .read = stdin_read,
		 .write = NULL
		},
		{.name = "stdout",
		 .open = stdout_open,
		 .close = stdout_close,
		 .read = NULL,
		 .write = stdout_write
		}
};

/* General handler */

struct VFSState {
	struct DevFileHandler *handler;
	void *context;
};

static int open(char *p) {
	COPY_STRING_TO_LOCAL(path, p);

	char *dirname;
	char *basename;
	split_path(path, &dirname, &basename);

	if (strcmp(dirname, "/dev") != 0)
		return -1;

	for (int i = 0; i < FILE_COUNT; ++i) {
		struct DevFileHandler *handler = &dev_file_handlers[i];
		if (strcmp(basename, handler->name) == 0) {
			void *context = handler->open();
			if ((int)context == -1)
				return -1;

			struct VFSState *state = kmalloc(sizeof(struct VFSState));
			state->handler = handler;
			state->context = context;

			int ft = register_file_table_context(state);
			return ft;
		}
	}

	return -1;
}

static int close(int ft) {
	struct VFSState *state = get_file_table_context(ft);
	kfree(get_file_table_context(ft));
	unregister_file_table_context(ft);
	if (state->handler->close == NULL)
		return -1;
	return state->handler->close(state->context);
}

static int read(int ft, char *buff, int size) {
	struct VFSState *state = get_file_table_context(ft);
	if (state->handler->read == NULL)
		return -1;
	return state->handler->read(state->context, buff, size);
}

static int write(int ft, char *buff, int size) {
	struct VFSState *state = get_file_table_context(ft);
	if (state->handler->write == NULL)
		return -1;
	return state->handler->write(state->context, buff, size);
}

static void file_stat(int index, struct VFSEntry *entry) {
	strcpy(entry->name, dev_file_handlers[index].name, MAX_VFS_NAME);
	entry->size = 0;
	entry->type = File;
}

static int stat(char *p, struct VFSEntry *entry) {
	COPY_STRING_TO_LOCAL(path, p);

	if (strcmp(path, "/dev") == 0) {
		strcpy(entry->name, "dev", MAX_VFS_NAME);
		entry->type = Directory;
		entry->size = 2;
		return 0;
	}

	char *dirname;
	char *basename;
	split_path(path, &dirname, &basename);

	if (strcmp(dirname, "/dev") != 0)
		return -1;

	for (int i = 0; i < FILE_COUNT; ++i) {
		if (strcmp(basename, dev_file_handlers[i].name) == 0) {
			file_stat(i, entry);
			return 0;
		}
	}

	return -1;
}

static int dirstat(char *p, struct VFSEntry *entries) {
	COPY_STRING_TO_LOCAL(path, p);

	if (strcmp(path, "/dev") != 0) {
		return -1;
	}

	for (int i = 0; i < FILE_COUNT; ++i) {
		file_stat(i, &entries[i]);
	}

	return 0;
}

struct VFSHandler dev_vfs = {
		.stat = stat,
		.dirstat = dirstat,

		.open = open,
		.close = close,

		.read = read,
		.write = write,

		.mkfile = NULL,
		.mkdir = NULL,

		.delete = NULL,

};
