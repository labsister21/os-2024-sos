#include "delete.h"
#include <path.h>
static int status;
void delete_recursive(char *init_path) {
	struct VFSEntry next_entry;
	status = syscall_VFS_STAT(init_path, &next_entry);
	if (status != 0) {
		puts("Error reading stat");
		return;
	}

	if (next_entry.type == Directory && next_entry.size != 0) {
		struct VFSEntry entries[next_entry.size];
		status = syscall_VFS_DIR_STAT(init_path, entries);
		if (status != 0) {
			puts("Error reading entries");
			return;
		}
		for (int i = 0; i < next_entry.size; i++) {
			char child[MAX_PATH];
			strcpy(child, init_path, MAX_PATH);
			strcat(child, "/", MAX_PATH);
			strcat(child, entries[i].name, MAX_PATH);
			// resolve_path(child);
			delete_recursive(child);
		}
	}

	status = syscall_VFS_DELETE(init_path);
	if (status != 0) {
		puts("Error deleting");
		return;
	}
	puts("File or directory deleted");
}