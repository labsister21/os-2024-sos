#include "header/filesystem/fat32.h"
#include "header/driver/disk.h"
#include "header/text/buffercolor.h"
#include "header/text/framebuffer.h"
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

		uint32_t next_cluster = free_clusters[i + 1];
		if (i + 1 == needed_cluster) {
			struct ClusterBuffer buf;
			next_cluster = FAT32_FAT_END_OF_FILE;
			int remainder = filesize % CLUSTER_SIZE;
			if (remainder != 0) {
				memcpy(&buf, src, remainder);
				for (int i = remainder; i < CLUSTER_SIZE; ++i)
					buf.buf[i] = 0x0;
				src = (void *)&buf;
			}
		}

		uint32_t cluster = free_clusters[i];
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
	int dir_entry_index;

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

// TODO: Read metadata method
