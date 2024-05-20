#ifndef _OLD_FAT32_H
#define _OLD_FAT32_H

#include <filesystem/fat32.h>

/* -- CRUD Operation -- */

/**
 *  FAT32 Folder / Directory read
 *
 * @param request buf point to struct FAT32DirectoryTable,
 *                name is directory name,
 *                ext is unused,
 *                parent_cluster_number is target directory table to read,
 *                buffer_size must be exactly sizeof(struct FAT32DirectoryTable)
 * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown
 */
int8_t read_directory(struct FAT32DriverRequest *request);

/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit
 * reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not
 * found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest *request);

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then
 * create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid
 * parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest *request);

/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in
 * file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1
 * unknown
 */
int8_t delete(struct FAT32DriverRequest *request);

#endif
