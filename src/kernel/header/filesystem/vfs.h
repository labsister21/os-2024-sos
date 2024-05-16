#ifndef _VFS_H
#define _VFS_H

#include <vfs.h>

struct VFSFileTableEntry;
/*
 * Parameter `int ft` will be translated
 * from local file descriptor for each process
 */
struct VFSHandler {
	int (*stat)(char *path, struct VFSEntry *entry);
	int (*dirstat)(char *path, struct VFSEntry *entries);

	int (*open)(char *path);
	int (*close)(int ft);

	int (*read)(int ft, char *buffer, int size);
	int (*write)(int ft, char *buffer, int size);

	int (*mkfile)(char *path);
	int (*mkdir)(char *path);

	int (*delete)(char *path);
};

int mount(char *path, struct VFSHandler *handler);

struct VFSHandler *get_handler_by_path(char *path);
struct VFSHandler *get_handler_by_fd(char *path);

struct VFSFileTableEntry {
	struct VFSHandler *handler;
};

int register_file_table(struct VFSFileTableEntry *entry);
int translate_fd_to_ft(int fd);
struct VFSFileTableEntry *get_vfs_table_entry(int ft);
int unregister_file_table(struct VFSFileTableEntry *entry);

#endif
