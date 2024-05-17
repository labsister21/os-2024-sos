#include "filesystem/vfs.h"
#include "memory/kmalloc.h"
#include <std/stdbool.h>
#include <std/string.h>

#define MAX_MOUNT 8
struct MountPoint {
	char *fullpath;
	char *parent;
	char *name;
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
	char *fullpath = kmalloc(size);
	strcpy(fullpath, path, size);

	int name_size = 0;
	int sep_idx = 0;
	for (int i = size - 1; i >= 0; --i) {
		if (fullpath[i] == '/') {
			sep_idx = i;
			break;
		};
		name_size += 1;
	}
	name_size += 1;

	char *name = kmalloc(name_size);
	strcpy(name, &fullpath[sep_idx + 1], name_size);

	int parent_size = size - name_size + 1;
	char *parent = kmalloc(parent_size);
	fullpath[sep_idx] = '\0';
	strcpy(parent, fullpath, parent_size);
	fullpath[sep_idx] = '/';
	if (sep_idx == 0)
		strcpy(parent, "/", 2);

	mount_point->fullpath = fullpath;
	mount_point->parent = parent;
	mount_point->name = name;
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
				strcmp(path, mount_points[idx].fullpath) == 0
		) break;

	end_loop:
		idx += 1;
	}
	if (idx == MAX_MOUNT) return -1;

	struct MountPoint *mount_point = &mount_points[idx];
	kfree(mount_point->fullpath);
	kfree(mount_point->parent);
	kfree(mount_point->name);
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

		int pattern_size = str_len(mount_point->fullpath);
		int match_count = count_match(path, mount_point->fullpath);

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
static struct VFSFileTableEntry *entries[MAX_FT];
static bool filled[MAX_FT];

struct VFSHandler *get_handler_by_file_table(int ft) {
	return entries[ft]->handler;
};

int register_file_table(struct VFSFileTableEntry *entry) {
	int idx = 0;
	while (idx < MAX_FT) {
		if (!filled[idx]) break;
		idx += 1;
	}
	if (idx == MAX_FT) return -1;

	filled[idx] = true;
	entries[idx] = entry;

	return idx;
};

// int translate_fd_to_ft(int fd);
struct VFSFileTableEntry *get_vfs_table_entry(int ft) {
	return entries[ft];
};

int unregister_file_table(struct VFSFileTableEntry *entry) {
	int idx = 0;
	while (idx < MAX_FT) {
		if (entries[idx] == entry) break;
		idx += 1;
	}
	if (idx == MAX_FT) return -1;

	filled[idx] = false;
	return 0;
};

/* Main API */

#define GET_HANDLER_BY_FT(ft)                                 \
	struct VFSHandler *handler = get_handler_by_file_table(ft); \
	if (handler == NULL)                                        \
		return -1;                                                \
	return handler

#define GET_HANDLER_BY_PATH(path)                         \
	struct VFSHandler *handler = get_handler_by_path(path); \
	if (handler == NULL)                                    \
		return -1;                                            \
	return handler

static int stat(char *path, struct VFSEntry *entry) {
	struct VFSHandler *handler = get_handler_by_path(path);
	if (handler == NULL)
		return -1;

	handler->stat(path, entry);
	if (entry->type == Directory) {
		for (int i = 0; i < MAX_MOUNT; ++i) {
			if (strcmp(path, mount_points[i].parent) == 0 && str_len(mount_points[i].name) != 0) {
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

	handler->dirstat(path, entries);

	struct VFSEntry entry;
	handler->stat(path, &entry);
	if (entry.type == Directory) {
		int count = entry.size;
		for (int i = 0; i < MAX_MOUNT; ++i) {
			struct MountPoint *mp = &mount_points[i];
			if (strcmp(path, mp->parent) == 0 && str_len(mp->name) != 0) {
				mp->handler->stat(mp->fullpath, &entries[count++]);
			}
		}
	}

	return 0;
};

static int open(char *path) { GET_HANDLER_BY_PATH(path)->open(path); };
static int close(int ft) { GET_HANDLER_BY_FT(ft)->close(ft); };

static int read(int ft, char *buffer, int size) { GET_HANDLER_BY_FT(ft)->read(ft, buffer, size); };
static int write(int ft, char *buffer, int size) { GET_HANDLER_BY_FT(ft)->write(ft, buffer, size); };

static int mkfile(char *path) { GET_HANDLER_BY_PATH(path)->mkfile(path); };
static int mkdir(char *path) { GET_HANDLER_BY_PATH(path)->mkdir(path); };

static int delete(char *path) { GET_HANDLER_BY_PATH(path)->delete (path); };

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
