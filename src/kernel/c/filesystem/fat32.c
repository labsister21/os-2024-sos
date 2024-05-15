#include "filesystem/fat32.h"
#include "driver/disk.h"
#include "text/framebuffer.h"
#include <fat32.h>
#include <std/stdbool.h>
#include <std/stdint.h>
#include <std/string.h>

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

static int8_t
get_dir_table_from_cluster(uint32_t cluster, struct FAT32DirectoryTable *dir_entry) {
	if (fat32_driver_state.fat_table.cluster_map[cluster] !=
			FAT32_FAT_END_OF_FILE)
		return -1;
	read_clusters(dir_entry, cluster, 1);
	if (strcmp(dir_entry->table[0].name, ".") == 0 &&
			dir_entry->table[0].attribute == ATTR_SUBDIRECTORY &&
			strcmp(dir_entry->table[1].name, "..") == 0 &&
			dir_entry->table[1].attribute == ATTR_SUBDIRECTORY)
		return 0;
	return -1;
}

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

int8_t read(struct FAT32DriverRequest *request) {
	struct FAT32DirectoryTable dir_table;
	// Parent folder not valid
	if (get_dir_table_from_cluster(request->parent_cluster_number, &dir_table) !=
			0)
		return -1;

	for (int i = 0; i < MAX_DIR_TABLE_ENTRY; ++i) {
		struct FAT32DirectoryEntry *dir_entry = &(dir_table.table[i]);
		if (dir_entry->user_attribute == UATTR_NOT_EMPTY &&
				strcmp(request->name, dir_entry->name) == 0 &&
				strcmp(request->ext, dir_entry->ext) == 0) {

			// Request file is a directory
			if (dir_entry->attribute == ATTR_SUBDIRECTORY)
				return 1;

			// Request buffer too small
			if (dir_entry->filesize > request->buffer_size)
				return -1;

			uint32_t cluster_number = get_cluster_from_dir_entry(dir_entry);
			void *ptr = request->buf;
			while (cluster_number != FAT32_FAT_END_OF_FILE) {
				read_clusters(ptr, cluster_number, 1);
				cluster_number =
						fat32_driver_state.fat_table.cluster_map[cluster_number];
				ptr += CLUSTER_SIZE;
			}

			return 0;
		}
	}

	// Request file not found
	return 2;
};

int8_t read_directory(struct FAT32DriverRequest *request) {
	struct FAT32DirectoryTable dir_table;
	// Parent folder not valid
	if (get_dir_table_from_cluster(request->parent_cluster_number, &dir_table) !=
			0)
		return -1;

	for (int i = 0; i < MAX_DIR_TABLE_ENTRY; ++i) {
		struct FAT32DirectoryEntry *dir_entry = &(dir_table.table[i]);

		if (dir_entry->user_attribute == UATTR_NOT_EMPTY &&
				strcmp(request->name, dir_entry->name) == 0) {
			// Request directory is a file
			if (dir_entry->attribute != ATTR_SUBDIRECTORY)
				return 1;

			// Request buffer too small
			if (CLUSTER_SIZE > request->buffer_size)
				return -1;

			uint32_t cluster_number = get_cluster_from_dir_entry(dir_entry);
			read_clusters(request->buf, cluster_number, 1);

			return 0;
		}
	}

	// Request directory not found
	return 2;
};

int8_t write(struct FAT32DriverRequest *request) {
	bool aFile = request->buffer_size > 0;

	struct FAT32DirectoryTable dir_table;
	// Parent folder not valid
	if (get_dir_table_from_cluster(request->parent_cluster_number, &dir_table) !=
			0)
		return 2;

	int dir_empty_entry = -1;
	for (int i = 0; i < MAX_DIR_TABLE_ENTRY; ++i) {
		struct FAT32DirectoryEntry *dir_entry = &(dir_table.table[i]);

		if (dir_empty_entry == -1 && dir_entry->user_attribute != UATTR_NOT_EMPTY)
			dir_empty_entry = i;

		// Entry exist
		if (dir_entry->user_attribute == UATTR_NOT_EMPTY &&
				strcmp(request->name, dir_entry->name) == 0 &&
				(!aFile || strcmp(request->ext, dir_entry->ext) == 0))
			return 1;
	}

	// Directory full
	if (dir_empty_entry == -1)
		return -1;

	uint32_t filesize =
			request->buffer_size == 0 ? CLUSTER_SIZE : request->buffer_size;
	int needed_cluster = (filesize + CLUSTER_SIZE - 1) / CLUSTER_SIZE; // Ceiled
	uint32_t free_clusters[needed_cluster];

	int cluster = 0;
	int free_cluster = 0;
	while (free_cluster < needed_cluster && cluster < CLUSTER_MAP_SIZE) {
		uint32_t cluster_state = fat32_driver_state.fat_table.cluster_map[cluster];
		if (cluster_state == FAT32_FAT_EMPTY_ENTRY) {
			free_clusters[free_cluster] = cluster;
			free_cluster++;
		}
		cluster++;
	}

	// FileAllocationTable full
	if (free_cluster < needed_cluster)
		return -1;

	// Writing to directory
	struct FAT32DirectoryEntry *dir_entry = &(dir_table.table[dir_empty_entry]);
	dir_entry->user_attribute = 0x0;
	dir_entry->filesize = filesize;
	dir_entry->attribute = aFile ? 0 : ATTR_SUBDIRECTORY;
	dir_entry->user_attribute = UATTR_NOT_EMPTY;
	dir_entry->cluster_low = (uint16_t)(free_clusters[0] & 0xFFFF);
	dir_entry->cluster_high = (uint16_t)(free_clusters[0] >> 16);
	strcpy(dir_entry->name, request->name, 8);
	if (aFile)
		memcpy(dir_entry->ext, request->ext, 3);
	write_clusters(&dir_table, request->parent_cluster_number, 1);

	struct FAT32DirectoryTable new_dir_table;
	void *src = request->buf;
	if (!aFile) {
		create_empty_directory_table(&new_dir_table, free_clusters[0], request->parent_cluster_number);
		src = (void *)&new_dir_table;
	}

	// Writing to cluster
	for (int i = 0; i < needed_cluster; ++i) {
		bool last = i + 1 == needed_cluster;
		uint32_t cluster = free_clusters[i];
		uint32_t next_cluster = last ? FAT32_FAT_END_OF_FILE : free_clusters[i + 1];

		if (last) {
			int remainder = filesize % CLUSTER_SIZE;
			struct ClusterBuffer buf;
			if (remainder != 0) {
				memcpy(&buf, src, remainder);
				memset(&buf.buf[remainder], 0x00, CLUSTER_SIZE - remainder);
				src = (void *)&buf;
			}
		}

		write_clusters(src, cluster, 1);
		src += CLUSTER_SIZE;
		fat32_driver_state.fat_table.cluster_map[cluster] = next_cluster;
	}
	write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);

	return 0;
};

int8_t delete(struct FAT32DriverRequest *request) {
	struct FAT32DirectoryTable dir_table;
	if (get_dir_table_from_cluster(request->parent_cluster_number, &dir_table) !=
			0)
		return -1;

	bool aFile;
	uint32_t cluster;
	int dir_entry_index = -1;

	for (int i = RESERVED_ENTRY; i < MAX_DIR_TABLE_ENTRY; ++i) {
		struct FAT32DirectoryEntry *dir_entry = &(dir_table.table[i]);
		aFile = dir_entry->attribute != ATTR_SUBDIRECTORY;
		if (dir_entry->user_attribute == UATTR_NOT_EMPTY &&
				strcmp(request->name, dir_entry->name) == 0 &&
				(!aFile || strcmp(request->ext, dir_entry->ext) == 0)) {
			cluster = get_cluster_from_dir_entry(dir_entry);
			dir_entry_index = i;
			break;
		}
	}

	if (dir_entry_index == -1) {
		return -1;
	}

	if (aFile) {
		while (cluster != FAT32_FAT_END_OF_FILE) {
			int next_cluster = fat32_driver_state.fat_table.cluster_map[cluster];
			fat32_driver_state.fat_table.cluster_map[cluster] = FAT32_FAT_EMPTY_ENTRY;
			cluster = next_cluster;
		}
	} else {
		struct FAT32DirectoryTable dir_table;
		read_clusters(&dir_table, cluster, 1);

		for (int i = RESERVED_ENTRY; i < MAX_DIR_TABLE_ENTRY; ++i) {
			if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY)
				return 2;
		}

		fat32_driver_state.fat_table.cluster_map[cluster] = FAT32_FAT_EMPTY_ENTRY;
	}

	memset(&dir_table.table[dir_entry_index], 0x00, sizeof(struct FAT32DirectoryEntry));

	write_clusters(&dir_table, request->parent_cluster_number, 1);
	write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
	return 0;
};

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

	char copy[str_len(path)];
	strcpy(copy, path, MAX_PATH);

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
		for (int i = 0; i < MAX_DIR_TABLE_ENTRY; ++i) {
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
struct VFSState { // Local struct to track opened file
	bool used;
	char buffer[CLUSTER_SIZE];
	uint32_t progress_pointer;
	uint32_t progress_end;
	uint32_t current_cluster;

	// Kinda botching here
	uint32_t directory_cluster;
	uint32_t directory_index;
};

struct VFSState vfs_state[MAX_OPENED_FILE];

static int open(char *path) {
	int idx = 0;
	while (idx < MAX_OPENED_FILE) {
		if (!vfs_state[idx].used) break;
		++idx;
	}

	if (idx == MAX_OPENED_FILE)
		return -1;

	struct VFSState *state = &vfs_state[idx];
	struct FAT32DirectoryEntry entry;
	int status = get_entry_with_parent_cluster_and_index(path, &entry, &state->directory_cluster, &state->directory_index);
	if (status != 0)
		return status;

	if (entry.attribute == ATTR_SUBDIRECTORY)
		return -1;

	state->current_cluster = get_cluster_from_dir_entry(&entry);
	state->progress_pointer = 0;
	state->progress_end = entry.filesize;
	state->used = true;

	return idx;
};

static int close(int fd) {
	if (!vfs_state[fd].used)
		return -1;

	vfs_state[fd].used = false;
	return 0;
};

static int read_vfs(int fd, char *buffer, int size) {
	struct VFSState *state = &vfs_state[fd];
	if (!state->used) // File closed
		return -1;

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

static int write_vfs(int fd, char *buffer, int size) {
	struct VFSState *state = &vfs_state[fd];
	if (!state->used) // File closed
		return -1;

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

int mkfile(char *path, char *name) {
	char *filename = strtok(name, '.');
	char *extension = strtok(NULL, '.');
	if (extension == NULL)
		extension = "";

	if (str_len(filename) > (8 - 1))
		return -1;

	if (str_len(extension) > (3 - 1))
		return -1;

	return mkgeneral(path, filename, extension, true);
}

int mkdir(char *path, char *name) {
	int i = 0;
	while (name[i] != '\0')
		if (name[i++] == '.') return -1;

	if (str_len(name) > (8 - 1))
		return -1;

	return mkgeneral(path, name, "", false);
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
