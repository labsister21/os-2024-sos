#ifndef _VFS_H
#define _VFS_H

struct VFSEntry {
	char name[255];
	int size;
	int type;
};

struct VFSHandler {
	int (*open)(char *);
	int (*close)(int fd);

	int (*read)(int fd);
	int (*write)(int fd);

	int (*stat)(char *, struct VFSEntry *);
	int (*dirstat)(char *, struct VFSEntry *);
};

#endif
