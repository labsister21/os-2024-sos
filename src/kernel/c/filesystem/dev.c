#include "filesystem/dev.h"
#include "memory/memory.h"
#include <path.h>
#include <std/stddef.h>

static int open(char *p) {
	COPY_STRING_TO_LOCAL(path, p);
	int ft = register_file_table_context(NULL);
	return ft;
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

	return 0;
}

static int dirstat(char *p, struct VFSEntry *entries) {
	COPY_STRING_TO_LOCAL(path, p);

	if (strcmp(path, "/dev") != 0) {
		return -1;
	}

	char *files[] = {"stdin", "stdout"};
	for (int i = 0; i < 2; ++i) {
		strcpy(entries[i].name, files[i], MAX_VFS_NAME);
		entries[i].size = 0;
		entries[i].type = File;
	}

	return 0;
}

struct VFSHandler dev_vfs = {
		.stat = stat,
		.dirstat = dirstat,

		.open = open,
		.close = NULL,

		.read = NULL,
		.write = NULL,

		.mkfile = NULL,
		.mkdir = NULL,

		.delete = NULL,

};
