#ifndef __VFS_H
#define __VFS_H

enum VFSType {
	File,
	Directory
};

struct VFSEntry {
	char name[255];
	int size;
	enum VFSType type;
};

#endif
