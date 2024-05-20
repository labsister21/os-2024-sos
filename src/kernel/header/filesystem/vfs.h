#ifndef _VFS_H
#define _VFS_H

#include <vfs.h>

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

// int translate_fd_to_ft(int fd);

int register_file_table_context(void *context);
void *get_file_table_context(int ft);
struct VFSHandler *get_file_table_handler(int ft);
int unregister_file_table_context(int ft);

extern struct VFSHandler vfs;

#endif
