#ifndef __FAT32_H
#define __FAT32_H

#include <disk.h>
#include <std/stdbool.h>
#include <std/stdint.h>

#define CLUSTER_BLOCK_COUNT 4
#define CLUSTER_SIZE (BLOCK_SIZE * CLUSTER_BLOCK_COUNT)
#define MAX_DIR_TABLE_ENTRY \
	(int)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry))

#define ROOT_CLUSTER_NUMBER 2

#define ATTR_SUBDIRECTORY 0b00010000
#define UATTR_NOT_EMPTY 0b10101010

/**
 * FAT32 standard 8.3 format - 32 bytes DirectoryEntry, Some detail can be found
 * at:
 * https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#Directory_entry,
 * and click show table.
 *
 * @param name           Entry name
 * @param ext            File extension
 * @param attribute      Will be used exclusively for subdirectory flag /
 * determining this entry is file or folder
 * @param user_attribute If this attribute equal with UATTR_NOT_EMPTY then entry
 * is not empty
 *
 * @param undelete       Unused / optional
 * @param create_time    Unused / optional
 * @param create_date    Unused / optional
 * @param access_time    Unused / optional
 * @param cluster_high   Upper 16-bit of cluster number
 *
 * @param modified_time  Unused / optional
 * @param modified_date  Unused / optional
 * @param cluster_low    Lower 16-bit of cluster number
 * @param filesize       Filesize of this file, if this is directory / folder,
 * filesize is 0
 */
struct FAT32DirectoryEntry {
	char name[8];
	char ext[3];
	uint8_t attribute;
	uint8_t user_attribute;

	bool undelete;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t access_date;
	uint16_t cluster_high;

	uint16_t modified_time;
	uint16_t modified_date;
	uint16_t cluster_low;
	uint32_t filesize;
} __attribute__((packed));

// FAT32 DirectoryTable, containing directory entry table - @param table Table
// of DirectoryEntry that span within 1 cluster
struct FAT32DirectoryTable {
	struct FAT32DirectoryEntry table[MAX_DIR_TABLE_ENTRY];
} __attribute__((packed));

/**
 * FAT32DriverRequest - Request for Driver CRUD operation
 *
 * @param buf                   Pointer pointing to buffer
 * @param name                  Name for directory entry
 * @param ext                   Extension for file
 * @param parent_cluster_number Parent directory cluster number, for updating
 * metadata
 * @param buffer_size           Buffer size, CRUD operation will have different
 * behaviour with this attribute
 */
struct FAT32DriverRequest {
	void *buf;
	char name[8];
	char ext[3];
	uint32_t parent_cluster_number;
	uint32_t buffer_size;
} __attribute__((packed));

static inline uint32_t get_cluster_from_dir_entry(struct FAT32DirectoryEntry *dir_entry) {
	return (dir_entry->cluster_low) + (((uint32_t)dir_entry->cluster_high) >> 16);
}

#endif
