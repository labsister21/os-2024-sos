#include "header/filesystem/fat32.h"
#include "header/driver/disk.h"
#include "header/stdlib/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

void write_clusters(
		const void *ptr, uint32_t cluster_number, uint8_t cluster_count
) {
	write_blocks(
			ptr, cluster_number * CLUSTER_BLOCK_COUNT,
			cluster_count * CLUSTER_BLOCK_COUNT
	);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
	read_blocks(
			ptr, cluster_number * CLUSTER_BLOCK_COUNT,
			cluster_count * CLUSTER_BLOCK_COUNT
	);
};

void create_empty_directory_table(
		struct FAT32DirectoryTable *dir_table, uint32_t current, uint32_t parent
) {
	memset(dir_table, 0x00, CLUSTER_SIZE);

	struct FAT32DirectoryEntry *current_entry = &(dir_table->table[0]);
	current_entry->name[0] = '.';
	current_entry->attribute = ATTR_SUBDIRECTORY;
	current_entry->user_attribute = UATTR_NOT_EMPTY;
	current_entry->cluster_low = (uint16_t)current;
	current_entry->cluster_high = (uint16_t)(current >> 16);

	struct FAT32DirectoryEntry *parent_entry = &(dir_table->table[1]);
	parent_entry->name[0] = '.';
	parent_entry->name[1] = '.';
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
	file_table.cluster_map[2] = ROOT_CLUSTER_NUMBER;
	write_clusters(&file_table, FAT_CLUSTER_NUMBER, 1);

	struct FAT32DirectoryTable dir_table;
	create_empty_directory_table(
			&dir_table, ROOT_CLUSTER_NUMBER, ROOT_CLUSTER_NUMBER
	);
	write_clusters(&dir_table, ROOT_CLUSTER_NUMBER, 1);
}

bool is_empty_storage() {
	struct BlockBuffer b;
	read_blocks(&b, 0, 1);
	for (int i = 0; i < BLOCK_SIZE; ++i) {
		if (fs_signature[i] != b.buf[i]) return true;
	}
	return false;
}

void initialize_filesystem_fat32() {
	if (is_empty_storage()) create_fat32();
	read_clusters(&fat32_driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
	read_clusters(&fat32_driver_state.dir_table_buf, ROOT_CLUSTER_NUMBER, 1);
}
