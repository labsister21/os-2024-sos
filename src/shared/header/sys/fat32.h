#include <std/stdbool.h>
#include <std/stdint.h>
#include <sys/disk.h>

#define CLUSTER_BLOCK_COUNT 4
#define CLUSTER_SIZE (BLOCK_SIZE * CLUSTER_BLOCK_COUNT)
#define MAX_DIR_TABLE_ENTRY \
	(int)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry))

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
