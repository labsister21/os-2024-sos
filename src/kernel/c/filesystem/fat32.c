#include "filesystem/fat32.h"
#include "driver/disk.h"
#include "memory/kmalloc.h"
#include <fat32.h>
#include <std/stdbool.h>
#include <std/stdint.h>
#include <std/string.h>

// TODO: path sanitizer

// clang-format off
const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '4', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};
// clang-format on

struct FAT32DriverState fat32_driver_state;

uint32_t cluster_to_lba(uint32_t cluster) {
	return cluster * CLUSTER_BLOCK_COUNT;
};

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
	write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
	read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
};

static void create_empty_directory_table(struct FAT32DirectoryTable *dir_table, uint32_t current, uint32_t parent) {
	memset(dir_table, 0x00, CLUSTER_SIZE);

	struct FAT32DirectoryEntry *current_entry = &(dir_table->table[0]);
	strcpy(current_entry->name, ".", 8);
	current_entry->attribute = ATTR_SUBDIRECTORY;
	current_entry->user_attribute = UATTR_NOT_EMPTY;
	current_entry->cluster_low = (uint16_t)current;
	current_entry->cluster_high = (uint16_t)(current >> 16);

	struct FAT32DirectoryEntry *parent_entry = &(dir_table->table[1]);
	strcpy(parent_entry->name, "..", 8);
	parent_entry->attribute = ATTR_SUBDIRECTORY;
	parent_entry->user_attribute = UATTR_NOT_EMPTY;
	parent_entry->cluster_low = (uint16_t)parent;
	parent_entry->cluster_high = (uint16_t)(parent >> 16);
};

void create_fat32() {
	write_blocks(fs_signature, 0, 1);
	struct FAT32FileAllocationTable file_table;
	for (int i = 0; i < CLUSTER_MAP_SIZE; ++i) {
		file_table.cluster_map[i] = FAT32_FAT_EMPTY_ENTRY;
	}
	file_table.cluster_map[0] = CLUSTER_0_VALUE;
	file_table.cluster_map[1] = CLUSTER_0_VALUE;
	file_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;
	write_clusters(&file_table, FAT_CLUSTER_NUMBER, 1);

	struct FAT32DirectoryTable dir_table;
	create_empty_directory_table(&dir_table, ROOT_CLUSTER_NUMBER, ROOT_CLUSTER_NUMBER);
	write_clusters(&dir_table, ROOT_CLUSTER_NUMBER, 1);
}

bool is_empty_storage() {
	struct BlockBuffer b;
	read_blocks(&b, 0, 1);
	for (int i = 0; i < BLOCK_SIZE; ++i) {
		if (fs_signature[i] != b.buf[i])
			return true;
	}
	return false;
}

void initialize_filesystem_fat32() {
	if (is_empty_storage())
		create_fat32();
	read_clusters(&fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
}

/* VFS Implementation */
#define MAX_83_FILENAME_SIZE 8 + 1 + 3 + 1
static void extract_83_fullname(struct FAT32DirectoryEntry *entry, char *result) {
	result[0] = '\0';
	strcat(result, entry->name, MAX_83_FILENAME_SIZE);

	if (entry->attribute != ATTR_SUBDIRECTORY && str_len(entry->ext) != 0) {
		strcat(result, ".", MAX_83_FILENAME_SIZE);
		strcat(result, entry->ext, MAX_83_FILENAME_SIZE);
	}
}

// Really botching here
#define MAX_PATH 1024
int get_entry_with_parent_cluster_and_index(char *path, struct FAT32DirectoryEntry *entry, uint32_t *parent_cluster, uint32_t *index) {
	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);

	char *current = strtok(copy, '/');

	struct FAT32DirectoryTable dir;
	read_clusters(&dir, ROOT_CLUSTER_NUMBER, 1);
	if (current == NULL) {
		*parent_cluster = ROOT_CLUSTER_NUMBER;
		memcpy(entry, &dir.table[0], sizeof(struct FAT32DirectoryEntry));
		return 0;
	}

	struct FAT32DirectoryEntry *current_entry = NULL;
	while (true) {
		*parent_cluster = get_cluster_from_dir_entry(&dir.table[0]);

		bool found = false;
		for (int i = RESERVED_ENTRY; i < MAX_DIR_TABLE_ENTRY; ++i) {
			current_entry = &dir.table[i];
			if (current_entry->user_attribute != UATTR_NOT_EMPTY)
				continue;

			char name[MAX_83_FILENAME_SIZE];
			extract_83_fullname(current_entry, name);
			if (strcmp(name, current) == 0) {
				*index = i;
				found = true;
				break;
			}
		}

		if (!found)
			return -1;

		current = strtok(NULL, '/');
		if (current == NULL) break;
		if (current_entry->attribute != ATTR_SUBDIRECTORY) return -1;
		read_clusters(&dir, get_cluster_from_dir_entry(current_entry), 1);
	}

	memcpy(entry, current_entry, sizeof(struct FAT32DirectoryEntry));
	return 0;
}

static int get_entry(char *path, struct FAT32DirectoryEntry *entry) {
	uint32_t tmp1, tmp2;
	return get_entry_with_parent_cluster_and_index(path, entry, &tmp1, &tmp2);
}

static int get_entry_count(struct FAT32DirectoryEntry *entry) {
	if (entry->attribute != ATTR_SUBDIRECTORY)
		return 0;

	int count = 0;
	struct FAT32DirectoryTable dir;
	read_clusters(&dir, get_cluster_from_dir_entry(entry), 1);
	for (int i = RESERVED_ENTRY; i < MAX_DIR_TABLE_ENTRY; ++i) {
		struct FAT32DirectoryEntry *current_entry = &dir.table[i];
		if (current_entry->user_attribute == UATTR_NOT_EMPTY)
			count += 1;
	}
	return count;
}

static void fat32_to_vfs(struct FAT32DirectoryEntry *fat32, struct VFSEntry *vfs) {
	bool aFile = fat32->attribute != ATTR_SUBDIRECTORY;

	extract_83_fullname(fat32, vfs->name);
	vfs->type = aFile ? File : Directory;
	vfs->size = fat32->filesize;
	if (!aFile)
		vfs->size = get_entry_count(fat32);
}

static int stat(char *path, struct VFSEntry *entry) {
	struct FAT32DirectoryEntry fat32_entry;
	int status = get_entry(path, &fat32_entry);
	if (status != 0)
		return status;

	fat32_to_vfs(&fat32_entry, entry);
	return 0;
};

static int dirstat(char *path, struct VFSEntry *entries) {
	struct FAT32DirectoryEntry fat32_entry;
	int status = get_entry(path, &fat32_entry);
	if (status != 0)
		return status;

	struct FAT32DirectoryTable dir;
	read_clusters(&dir, get_cluster_from_dir_entry(&fat32_entry), 1);

	int count = 0;
	for (int i = RESERVED_ENTRY; i < MAX_DIR_TABLE_ENTRY; ++i) {
		struct FAT32DirectoryEntry *current_entry = &dir.table[i];
		if (current_entry->user_attribute != UATTR_NOT_EMPTY)
			continue;

		fat32_to_vfs(current_entry, &entries[count++]);
	}

	return 0;
};

#define MAX_OPENED_FILE 16
struct VFSStateOld { // Local struct to track opened file
	bool used;
	char buffer[CLUSTER_SIZE];
	uint32_t progress_pointer;
	uint32_t progress_end;
	uint32_t current_cluster;

	// Kinda botching here
	uint32_t directory_cluster;
	uint32_t directory_index;
};

// Local struct to track opened file
struct VFSState {
	struct VFSFileTableEntry entry;

	char buffer[CLUSTER_SIZE];
	uint32_t progress_pointer;
	uint32_t progress_end;
	uint32_t current_cluster;

	// Kinda botching here
	uint32_t directory_cluster;
	uint32_t directory_index;
};

static int open(char *path) {
	struct VFSState *state = kmalloc(sizeof(struct VFSState));
	state->entry.handler = &fat32_vfs;

	struct FAT32DirectoryEntry entry;
	int status = get_entry_with_parent_cluster_and_index(path, &entry, &state->directory_cluster, &state->directory_index);
	if (status != 0)
		return status;

	if (entry.attribute == ATTR_SUBDIRECTORY)
		return -1;

	state->current_cluster = get_cluster_from_dir_entry(&entry);
	state->progress_pointer = 0;
	state->progress_end = entry.filesize;

	status = register_file_table((void *)state);
	if (status < 0) {
		kfree(state);
		return status;
	}

	return status;
};

static int close(int ft) {
	struct VFSFileTableEntry *entry = get_vfs_table_entry(ft);
	unregister_file_table(entry);
	kfree(entry);
	return 0;
};

static int read_vfs(int ft, char *buffer, int size) {
	struct VFSState *state = (void *)get_vfs_table_entry(ft);

	int read_count = 0;
	while (true) {
		if (read_count >= size) break;
		if (state->progress_pointer >= state->progress_end) break;

		int local_offset = state->progress_pointer % CLUSTER_SIZE;
		if (local_offset == 0) {
			if (state->current_cluster == FAT32_FAT_END_OF_FILE) // Corrupted file
				return -1;

			read_clusters(state->buffer, state->current_cluster, 1);
			state->current_cluster = fat32_driver_state.fat_table.cluster_map[state->current_cluster];
		}

		buffer[read_count] = state->buffer[local_offset];
		state->progress_pointer += 1;
		read_count += 1;
	}

	return read_count;
};

static int write_vfs(int ft, char *buffer, int size) {
	struct VFSState *state = (void *)get_vfs_table_entry(ft);

	int write_count = 0;
	while (true) {
		if (write_count >= size) break;

		int local_offset = state->progress_pointer % CLUSTER_SIZE;
		if (local_offset == 0) {
			uint32_t old_cluster = state->current_cluster;
			if (state->progress_pointer != 0) {
				write_clusters(state->buffer, state->current_cluster, 1);
				state->current_cluster = fat32_driver_state.fat_table.cluster_map[state->current_cluster];
			}

			if (state->current_cluster == FAT32_FAT_END_OF_FILE) {
				// Searching free FAT table
				uint32_t free_cluster = 0;
				while (free_cluster < CLUSTER_MAP_SIZE) {
					if (fat32_driver_state.fat_table.cluster_map[free_cluster] == FAT32_FAT_EMPTY_ENTRY) break;
					free_cluster += 1;
				}

				if (free_cluster == CLUSTER_MAP_SIZE) // Storage full
					return -1;

				// Refreshing FAT table
				fat32_driver_state.fat_table.cluster_map[old_cluster] = free_cluster;
				fat32_driver_state.fat_table.cluster_map[free_cluster] = FAT32_FAT_END_OF_FILE;
				write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
				state->current_cluster = free_cluster;
			}

			read_clusters(state->buffer, state->current_cluster, 1);
		}

		state->buffer[local_offset] = buffer[write_count];
		state->progress_pointer += 1;
		write_count += 1;
	}
	write_clusters(state->buffer, state->current_cluster, 1);

	if (state->progress_pointer > state->progress_end)
		state->progress_end = state->progress_pointer;

	// Refresh directory table
	struct FAT32DirectoryTable dir;
	read_clusters(&dir, state->directory_cluster, 1);
	dir.table[state->directory_index].filesize = state->progress_end;
	write_clusters(&dir, state->directory_cluster, 1);

	return write_count;
};

int mkgeneral(char *path, char *name, char *ext, bool aFile) {
	if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
		return -1;

	struct FAT32DirectoryEntry entry;
	int status = get_entry(path, &entry);
	if (status != 0)
		return status;

	struct FAT32DirectoryTable dir;
	uint32_t parent_cluster = get_cluster_from_dir_entry(&entry);
	read_clusters(&dir, parent_cluster, 1);

	bool duplicate = false;
	struct FAT32DirectoryEntry *empty_entry = NULL;

	char used_name[MAX_83_FILENAME_SIZE];
	for (int i = RESERVED_ENTRY; i < MAX_DIR_TABLE_ENTRY; ++i) {
		struct FAT32DirectoryEntry *current_entry = &dir.table[i];
		if (current_entry->user_attribute == UATTR_NOT_EMPTY) {
			extract_83_fullname(current_entry, used_name);
			if (strcmp(name, used_name) == 0) {
				duplicate = true;
				break;
			}
		} else if (empty_entry == NULL) empty_entry = current_entry;
	}

	if (duplicate || empty_entry == NULL)
		return -1;

	uint32_t free_cluster = 0;
	while (free_cluster < CLUSTER_MAP_SIZE) {
		if (fat32_driver_state.fat_table.cluster_map[free_cluster] == FAT32_FAT_EMPTY_ENTRY) break;
		free_cluster += 1;
	}

	if (free_cluster == CLUSTER_MAP_SIZE)
		return -1;

	empty_entry->user_attribute = 0x0;
	empty_entry->filesize = 0;
	empty_entry->attribute = aFile ? 0 : ATTR_SUBDIRECTORY;
	empty_entry->user_attribute = UATTR_NOT_EMPTY;
	empty_entry->cluster_low = (uint16_t)(free_cluster & 0xFFFF);
	empty_entry->cluster_high = (uint16_t)(free_cluster >> 16);
	strcpy(empty_entry->name, name, 8);
	if (aFile)
		strcpy(empty_entry->ext, ext, 3);
	write_clusters(&dir, parent_cluster, 1);

	fat32_driver_state.fat_table.cluster_map[free_cluster] = FAT32_FAT_END_OF_FILE;
	write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);

	if (!aFile) {
		struct FAT32DirectoryTable new_dir_table;
		create_empty_directory_table(&new_dir_table, free_cluster, parent_cluster);
		write_clusters(&new_dir_table, free_cluster, 1);
	}

	return 0;
}

static void parse_path(char *path, char **parent, char **name) {
	int size = str_len(path);

	int i = size - 1;
	while (i >= 0) {
		if (path[i] == '/') break;
		i -= 1;
	}

	if (i == -1) {
		*parent = &path[size];
		*name = path;
	} else {
		path[i] = '\0';
		*parent = path;
		*name = &path[i + 1];
	}
}

int mkfile(char *path) {
	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);

	char *parent;
	char *name;
	parse_path(copy, &parent, &name);

	char *filename = strtok(name, '.');
	char *extension = strtok(NULL, '.');
	if (extension == NULL)
		extension = "";

	if (str_len(filename) > (8 - 1))
		return -1;

	if (str_len(extension) > (3 - 1))
		return -1;

	return mkgeneral(parent, filename, extension, true);
}

int mkdir(char *path) {
	int size = str_len(path) + 1;
	char copy[size];
	strcpy(copy, path, size);

	char *parent;
	char *dir;
	parse_path(path, &parent, &dir);

	int i = 0;
	while (dir[i] != '\0')
		if (dir[i++] == '.') return -1;

	if (str_len(dir) > (8 - 1))
		return -1;

	return mkgeneral(parent, dir, "", false);
}

int delete_vfs(char *path) {
	uint32_t parent_cluster;
	uint32_t index;

	struct FAT32DirectoryEntry entry;
	int status = get_entry_with_parent_cluster_and_index(path, &entry, &parent_cluster, &index);
	if (status != 0)
		return status;

	if (get_entry_count(&entry) != 0)
		return -1;

	uint32_t current_cluster = get_cluster_from_dir_entry(&entry);
	while (current_cluster != FAT32_FAT_END_OF_FILE) {
		int next_cluster = fat32_driver_state.fat_table.cluster_map[current_cluster];
		fat32_driver_state.fat_table.cluster_map[current_cluster] = FAT32_FAT_EMPTY_ENTRY;
		current_cluster = next_cluster;
	}

	struct FAT32DirectoryTable dir;
	read_clusters(&dir, parent_cluster, 1);
	memset(&dir.table[index], 0x00, sizeof(struct FAT32DirectoryEntry));

	write_clusters(&dir, parent_cluster, 1);
	write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);

	return 0;
};

struct VFSHandler fat32_vfs = {
		.stat = stat,
		.dirstat = dirstat,

		.open = open,
		.close = close,

		.read = read_vfs,
		.write = write_vfs,

		.mkfile = mkfile,
		.mkdir = mkdir,

		.delete = delete_vfs,
};
