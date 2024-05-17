#ifndef __VFS_H
#define __VFS_H

#define MAX_VFS_NAME 255
enum VFSType {
	File,
	Directory
};

struct VFSEntry {
	char name[MAX_VFS_NAME];
	int size;
	enum VFSType type;
};

#endif
