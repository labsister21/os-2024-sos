#ifndef _VFS_H
#define _VFS_H

enum VFSType {
	File,
	Directory
};

struct VFSEntry {
	char name[255];
	int size;
	enum VFSType type;
};

struct VFSHandler {
	int (*stat)(char *path, struct VFSEntry *entry);
	int (*dirstat)(char *path, struct VFSEntry *entries);

	int (*open)(char *path);
	int (*close)(int fd);

	int (*read)(int fd, char *buffer, int size);
	int (*write)(int fd, char *buffer, int size);

	int (*mkfile)(char *path, char *name);
	int (*mkdir)(char *path, char *name);
};

#endif
