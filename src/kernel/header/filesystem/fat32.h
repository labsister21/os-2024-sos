#ifndef _FAT32_H
#define _FAT32_H

#include "driver/disk.h"
#include "filesystem/vfs.h"
#include <fat32.h>
#include <std/stdbool.h>
#include <std/stdint.h>

/**
 * FAT32 - IF2230 edition - 2024
 */

/* -- IF2230 File System constants -- */
#define BOOT_SECTOR 0
#define CLUSTER_MAP_SIZE 512

/* -- FAT32 FileAllocationTable constants -- */
// FAT reserved value for cluster 0 and 1 in FileAllocationTable
#define CLUSTER_0_VALUE 0x0FFFFFF0
#define CLUSTER_1_VALUE 0x0FFFFFFF

// EOF also double as valid cluster / "this is last valid cluster in the chain"
#define FAT32_FAT_EMPTY_ENTRY 0x00000000
#define FAT32_FAT_END_OF_FILE 0x0FFFFFFF

#define FAT_CLUSTER_NUMBER 1

/* -- FAT32 DirectoryEntry constants -- */
#define RESERVED_ENTRY 2

// Boot sector signature for this file system "FAT32 - IF2230 edition"
extern const uint8_t fs_signature[BLOCK_SIZE];

// Cluster buffer data type - @param buf Byte buffer with size of CLUSTER_SIZE
struct ClusterBuffer {
	uint8_t buf[CLUSTER_SIZE];
} __attribute__((packed));

/* -- FAT32 Data Structures -- */

/**
 * FAT32 FileAllocationTable, for more information about this, check guidebook
 *
 * @param cluster_map Containing cluster map of FAT32
 */
struct FAT32FileAllocationTable {
	uint32_t cluster_map[CLUSTER_MAP_SIZE];
} __attribute__((packed));

/* -- FAT32 Driver -- */

/**
 * FAT32DriverState - Contain all driver states
 *
 * @param fat_table     FAT of the system, will be loaded during
 * initialize_filesystem_fat32()
 * @param cluster_buf   Buffer for cluster, can be used for temp var
 */
struct FAT32DriverState {
	struct FAT32FileAllocationTable fat_table;
	struct ClusterBuffer cluster_buf;
} __attribute__((packed));
extern struct FAT32DriverState fat32_driver_state;

/* -- Driver Interfaces -- */

/**
 * Convert cluster number to logical block address
 *
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster);

/**
 * Checking whether filesystem signature is missing or not in boot sector
 *
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void);

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE,
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void);

/**
 * Initialize file system driver state, if is_empty_storage() then
 * create_fat32() Else, read and cache entire FileAllocationTable (located at
 * cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void);

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 *
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks
 * block_count 255 => max cluster_count = 63
 */
void write_clusters(
		const void *ptr, uint32_t cluster_number, uint8_t cluster_count
);

/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 *
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks
 * block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count);

#endif
