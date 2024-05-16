#ifndef _VFS_H
#define _VFS_H

#include <vfs.h>
struct VFSHandler {
	int (*stat)(char *path, struct VFSEntry *entry);
	int (*dirstat)(char *path, struct VFSEntry *entries);

	int (*open)(char *path);
	int (*close)(int fd);

	int (*read)(int fd, char *buffer, int size);
	int (*write)(int fd, char *buffer, int size);

	int (*mkfile)(char *path);
	int (*mkdir)(char *path);

	int (*delete)(char *path);
};

int mount(char *path, struct VFSHandler *handler);

struct VFSHandler *get_handler_by_path(char *path);
struct VFSHandler *get_handler_by_fd(char *path);

#endif
