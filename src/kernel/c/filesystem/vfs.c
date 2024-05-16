#include "filesystem/vfs.h"
#include "memory/kmalloc.h"
#include <std/stdbool.h>
#include <std/string.h>

#define MAX_MOUNT 8
struct MountPoint {
	char *path;
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
	char *copy = kmalloc(size);
	strcpy(copy, path, size);

	mount_point->path = copy;
	mount_point->handler = handler;
	mount_point->filled = true;

	return 0;
};

int unmount(char *path, struct VFSHandler *handler) {
	int idx = 0;
	while (idx < MAX_MOUNT) {
		if (!mount_points[idx].filled) continue;
		if (
				mount_points[idx].handler == handler &&
				strcmp(path, mount_points[idx].path) == 0
		) break;
		idx += 1;
	}
	if (idx == MAX_MOUNT) return -1;

	struct MountPoint *mount_point = &mount_points[idx];
	kfree(mount_point->path);
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
