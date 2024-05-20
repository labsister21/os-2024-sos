#include "filesystem/vfs.h"
#include "memory/kmalloc.h"
#include <path.h>
#include <std/stdbool.h>
#include <std/string.h>
#define MAX_MOUNT 8
struct MountPoint {
	char *path;
	char *dirname;
	char *basename;
	struct VFSHandler *handler;
	bool filled;
};
static struct MountPoint mount_points[MAX_MOUNT];

int mount(char *path, struct VFSHandler *handler) {
	int idx = 0;
	while (idx < MAX_MOUNT) {
		if (!mount_points[idx].filled) break;
		idx += 1;
	}
	if (idx == MAX_MOUNT) return -1;

	struct MountPoint *mount_point = &mount_points[idx];

	int size = str_len(path) + 1;
	char *path_copy = kmalloc(size);
	strcpy(path_copy, path, size);

	char path_tmp[size];
	strcpy(path_tmp, path, size);

	char *dirname;
	char *basename;
	split_path(path_tmp, &dirname, &basename);

	int dirname_size = str_len(dirname) + 1;
	int basename_size = str_len(basename) + 1;
	mount_point->dirname = kmalloc(dirname_size);
	mount_point->basename = kmalloc(basename_size);
	strcpy(mount_point->dirname, dirname, dirname_size);
	strcpy(mount_point->basename, basename, basename_size);

	mount_point->path = path_copy;
	mount_point->handler = handler;
	mount_point->filled = true;

	return 0;
};

int unmount(char *path, struct VFSHandler *handler) {
	int idx = 0;
	while (idx < MAX_MOUNT) {
		if (!mount_points[idx].filled) goto end_loop;
		if (
				mount_points[idx].handler == handler &&
				strcmp(path, mount_points[idx].path) == 0
		) break;

	end_loop:
		idx += 1;
	}
	if (idx == MAX_MOUNT) return -1;

	struct MountPoint *mount_point = &mount_points[idx];
	kfree(mount_point->path);
	kfree(mount_point->dirname);
	kfree(mount_point->basename);
	mount_point->filled = false;

	return 0;
};

static int count_match(char *text, char *pattern) {
	int i = 0;
	while (text[i] != '\0' && pattern[i] != '\0' && text[i] == pattern[i])
		i += 1;
	return i;
}

struct VFSHandler *get_handler_by_path(char *path) {
	int max_match = -1;
	struct VFSHandler *result = NULL;
	int idx = 0;

	while (idx < MAX_MOUNT) {
		struct MountPoint *mount_point = &mount_points[idx];

		if (!mount_point->filled) goto end_loop;

		int pattern_size = str_len(mount_point->path);
		int match_count = count_match(path, mount_point->path);

		if (match_count != pattern_size) goto end_loop;
		if (!(match_count > max_match)) goto end_loop;

		result = mount_point->handler;
		max_match = match_count;

	end_loop:
		idx += 1;
	}

	return result;
};

#define MAX_FT 128

static struct VFSHandler *last_handler;
static void *file_table_context[MAX_FT];
static struct VFSHandler *file_table_handler[MAX_FT];

int register_file_table_context(void *context) {
	int ft = 0;
	while (ft < MAX_FT) {
		if (file_table_context[ft] == NULL) break;
		ft += 1;
	}

	file_table_context[ft] = context;
	file_table_handler[ft] = last_handler;
	return ft;
}

void *get_file_table_context(int ft) {
	return file_table_context[ft];
}

struct VFSHandler *get_file_table_handler(int ft) {
	return file_table_handler[ft];
}

int unregister_file_table_context(int ft) {
	file_table_context[ft] = NULL;
	file_table_handler[ft] = NULL;
	return 0;
}

/* Main API */

#define RUN_HANDLER(result, name, get_handler, param_handler, ...) \
	struct                                                           \
			VFSHandler *handler = get_handler(param_handler);            \
	if (handler == NULL)                                             \
		return -1;                                                     \
	if (handler->name == NULL)                                       \
		return -1;                                                     \
	result = handler->name(__VA_ARGS__);

#define DIRECT_RUN_HANDLER(name, get_handler, param_handler, ...)    \
	int result;                                                        \
	RUN_HANDLER(result, name, get_handler, param_handler, __VA_ARGS__) \
	return result;

static int stat(char *path, struct VFSEntry *entry) {
	int result;
	RUN_HANDLER(result, stat, get_handler_by_path, path, path, entry);
	if (result != 0)
		return -1;

	if (entry->type == Directory) {
		for (int i = 0; i < MAX_MOUNT; ++i) {
			if (
					strcmp(path, mount_points[i].dirname) == 0 &&
					str_len(mount_points[i].basename) != 0
			) {
				entry->size += 1;
			}
		}
	}

	return 0;
};

static int dirstat(char *path, struct VFSEntry *entries) {
	struct VFSHandler *handler = get_handler_by_path(path);
	if (handler == NULL)
		return -1;
	(void)entries;

	int status;
	struct VFSEntry entry;
	if (handler->stat == NULL)
		return -1;
	status = handler->stat(path, &entry);
	if (status != 0)
		return status;

	if (handler->dirstat == NULL)
		return -1;
	status = handler->dirstat(path, entries);
	if (status != 0)
		return status;

	if (entry.type == Directory) {
		int original_count = entry.size;
		int count = original_count;
		for (int i = 0; i < MAX_MOUNT; ++i) {
			struct MountPoint *mp = &mount_points[i];
			if (
					strcmp(path, mp->dirname) == 0 &&
					str_len(mp->basename) != 0
			) {
				if (mp->handler->stat)
					mp->handler->stat(mp->path, &entries[count++]);
			}
		}
	}

	return 0;
};

static int open(char *path) {
	last_handler = get_handler_by_path(path);
	if (last_handler == NULL || last_handler->open == NULL)
		return -1;

	int result = last_handler->open(path);
	last_handler = NULL;
	return result;
};

static int close(int ft){DIRECT_RUN_HANDLER(close, get_file_table_handler, ft, ft)};

static int read(int ft, char *buffer, int size){DIRECT_RUN_HANDLER(read, get_file_table_handler, ft, ft, buffer, size)};
static int write(int ft, char *buffer, int size){DIRECT_RUN_HANDLER(write, get_file_table_handler, ft, ft, buffer, size)};

static int mkfile(char *path) { DIRECT_RUN_HANDLER(mkfile, get_handler_by_path, path, path); };
static int mkdir(char *path) { DIRECT_RUN_HANDLER(mkdir, get_handler_by_path, path, path); };

static int delete(char *path) { DIRECT_RUN_HANDLER(delete, get_handler_by_path, path, path); };

struct VFSHandler vfs = {
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
