#include <fat32.h>
#include <std/stdint.h>

uint32_t get_cluster_from_dir_entry(struct FAT32DirectoryEntry *dir_entry) {
	return (dir_entry->cluster_low) + (((uint32_t)dir_entry->cluster_high) >> 16);
}
