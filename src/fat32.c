#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/stdlib/string.h"
#include "header/filesystem/fat32.h"
#include "header/driver/disk.h"

static struct FAT32DriverState fat32_driver_state;

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '4', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

uint32_t cluster_to_lba(uint32_t cluster) {
    return cluster * CLUSTER_BLOCK_COUNT;
}

uint16_t merge_cluster(uint16_t low, uint16_t high) {
    return (uint16_t)(low) + (uint16_t)(high << 16);
}

int ceil(int pembilang, int penyebut) {
    return (pembilang+penyebut-1)/penyebut;
    
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster, uint32_t current_dir_cluster) {
    memset(dir_table, 0x00, CLUSTER_SIZE);
    struct FAT32DirectoryEntry *self_entry = &dir_table->table[0];
    strcpy(self_entry->name, name);
    self_entry->attribute = ATTR_SUBDIRECTORY;
    self_entry->user_attribute = UATTR_NOT_EMPTY;
    self_entry->cluster_high = (uint16_t)(current_dir_cluster >> 16);
    self_entry->cluster_low = (uint16_t)(current_dir_cluster);
    
    struct FAT32DirectoryEntry *parent_entry = &dir_table->table[1];
    strcpy(parent_entry->name, "..");
    parent_entry->attribute = ATTR_SUBDIRECTORY;
    parent_entry->user_attribute = UATTR_NOT_EMPTY;
    parent_entry->cluster_high = (uint16_t)(parent_dir_cluster >> 16);
    parent_entry->cluster_low = (uint16_t)(parent_dir_cluster);
}

bool is_empty_storage(void) {
    struct BlockBuffer b;
    read_blocks(&b, BOOT_SECTOR, 1);
    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        if (fs_signature[i] != b.buf[i])
        {
            return true;
        }
    }
    return false;
    
}

void create_fat32(void) {
    write_blocks(fs_signature, BOOT_SECTOR, 1);
    fat32_driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    fat32_driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    fat32_driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;
    for (int i = 3; i < CLUSTER_MAP_SIZE; i++)
    {
        fat32_driver_state.fat_table.cluster_map[i] = FAT32_FAT_EMPTY_ENTRY;
    }
    write_clusters(&fat32_driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    init_directory_table(&fat32_driver_state.dir_table_buf, "root", ROOT_CLUSTER_NUMBER, ROOT_CLUSTER_NUMBER);
    write_clusters(&fat32_driver_state.dir_table_buf, ROOT_CLUSTER_NUMBER, 1);
};

void initialize_filesystem_fat32(void) {
    if (is_empty_storage())
    {
        create_fat32();
    }
    read_clusters(&fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
};

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count*CLUSTER_BLOCK_COUNT);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count*CLUSTER_BLOCK_COUNT);
}

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
int8_t read_directory(struct FAT32DriverRequest request) {
    read_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);

    for (uint32_t i = 0; i < DIRECTORY_TABLE_SIZE; i++)
    {
        if (memcmp(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8) == 0)
        {
            if (fat32_driver_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY)
            {
                read_clusters(&request.buf, fat32_driver_state.dir_table_buf.table[i].cluster_low, 1);
                return 0;
            }
            else
            {
                return 1;
            }
        }
    }
    return 2;
}


/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable *dir_table = &fat32_driver_state.dir_table_buf;
    bool isFound = false;
    int idx = -1;

    read_clusters(dir_table, request.parent_cluster_number, 1);

    for (uint32_t i = 0; i < DIRECTORY_TABLE_SIZE; i++)
    {
        if (strcmp(dir_table->table[i].name, request.name, 8) == 0) 
        {
            // File
            isFound = true;
            if (strcmp(dir_table->table[i].ext, request.ext, 3) == 0)
            {
                if (request.buffer_size < dir_table->table[i].filesize)
                {
                    return 2;
                }
                else
                {
                    idx = i;
                }
            }
        }
    }

    // Not found
    if (idx == -1 && !isFound)
    {
        return 3;
    }
    
    // Check the entry is file or folder and check if the file valid
    if (idx != -1)
    {
        struct FAT32FileAllocationTable *fat_table = &fat32_driver_state.fat_table;
        read_clusters(fat_table, FAT_CLUSTER_NUMBER, 1);
        uint32_t real_cluster = merge_cluster(dir_table->table[idx].cluster_low, dir_table->table[idx].cluster_high);
        read_clusters(&request.buf, real_cluster, 1);
        uint32_t cluster = fat_table->cluster_map[real_cluster];
        int i = 0;
        while (cluster != FAT32_FAT_END_OF_FILE)
        {
            read_clusters(&request.buf+CLUSTER_SIZE*i, cluster, 1);
            cluster = fat_table->cluster_map[cluster];
            framebuffer_write(0,3, i + '0', 0xF, 0);    
            i++;
        }
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable *dir_table = &fat32_driver_state.dir_table_buf;
    read_clusters(dir_table, request.parent_cluster_number, 1);

    // Check parent valid or not
    if (strcmp(dir_table->table[1].name, "..", 2) != 0)
    {
        return 2;
    }

    // Check the file/folder already exist or not
    for (uint32_t i = 0; i < DIRECTORY_TABLE_SIZE; i++)
    {
        if (strcmp(dir_table->table[i].name, request.name, 8) == 0) 
        {
            // File
            if (strcmp(dir_table->table[i].ext, request.ext, 3) == 0)
            {
                return 1;
            }
        }
    }

    // Check the cluster is available or not
    int needed_cluster = ceil(request.buffer_size,CLUSTER_SIZE);
    int count = 0;
    int idx = 0;
    int j = 0;
    uint32_t freeClusters[needed_cluster];
    while (j < CLUSTER_MAP_SIZE && count < needed_cluster) {
        uint32_t cluster_state = fat32_driver_state.fat_table.cluster_map[j];
        if (cluster_state == FAT32_FAT_EMPTY_ENTRY)
        {
            count++;
            freeClusters[idx] = j;
        }
        j++;
    }
    if (count < needed_cluster)
    {
        return -1;
    }
    
    // Search empty directory table
    int empty_dir_index = -1;
    for (uint32_t i = 0; i < (DIRECTORY_TABLE_SIZE) && empty_dir_index == -1; i++)
    {
        if (dir_table->table[i].user_attribute != UATTR_NOT_EMPTY)
        {
            empty_dir_index = i;
        }
    }
    if (empty_dir_index == -1)
    {
        return -1;
    }
    
    
    // Write to directory table
    struct FAT32DirectoryEntry *write_entry = &(dir_table->table[empty_dir_index]);
    strcpy(write_entry->name, request.name);
    write_entry->cluster_high = (uint16_t)(freeClusters[0] >> 16);
    write_entry->cluster_low = (uint16_t)(freeClusters[0] & 0xFFFF);
    write_entry->filesize = request.buffer_size;
    if (request.buffer_size == 0)
    {
        write_entry->attribute = ATTR_SUBDIRECTORY; 
    }
    else
    {
        write_entry->attribute = 0;
        strcpy(write_entry->ext, request.ext);
    }
    if (strcmp(request.name, "nigger", 8) == 0)
    {
        framebuffer_write(0,5, empty_dir_index + '0', 0xF, 0);
        framebuffer_write(1,2, write_entry->cluster_low + '0', 0xF, 0);    
        framebuffer_write(1,3, write_entry->cluster_high + '0', 0xF, 0);    
    }
    if (strcmp(request.name, "con", 8) == 0)
    {
        framebuffer_write(0,7, empty_dir_index + '0', 0xF, 0);
        framebuffer_write(1,6, write_entry->cluster_low + '0', 0xF, 0);    
        framebuffer_write(1,7, write_entry->cluster_high + '0', 0xF, 0);    
    }
    write_entry->user_attribute = UATTR_NOT_EMPTY;  // Ini kalo disimpen di atas bakal error, tapi kalo disini bener (mungkin)
    framebuffer_write(0,10, write_entry->user_attribute + '0', 0xF, 0);

    write_clusters(dir_table->table, request.parent_cluster_number, 1);
    
    struct FAT32DirectoryTable new_dir_table;
    void *src = request.buf;
    if (request.buffer_size == 0)
    {
        init_directory_table(&fat32_driver_state.dir_table_buf, request.name, request.parent_cluster_number, freeClusters[0]);
        src = (void *)&new_dir_table;
    }

    uint32_t filesize = request.buffer_size == 0 ? CLUSTER_SIZE : request.buffer_size;
    // write file and directory
    for (int i = 0; i < needed_cluster; i++)
    {
        uint32_t next_cluster = freeClusters[i+1];
        if (i + 1 == needed_cluster)
        {
            struct ClusterBuffer buf;
            next_cluster = FAT32_FAT_END_OF_FILE;
            int remainder = filesize % CLUSTER_SIZE;
            if (remainder != 0)
            {
                memcpy(&buf, src, remainder);
                memset(&buf.buf[remainder], 0x00, CLUSTER_SIZE - remainder);
                src = (void *)&buf;
            } 
        }
        write_clusters(src+i*CLUSTER_SIZE, freeClusters[i], 1);
        src += CLUSTER_SIZE;
        fat32_driver_state.fat_table.cluster_map[freeClusters[i]] = next_cluster; 
    }
    write_clusters(&fat32_driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);    
    return 0;
}


/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable *dir_table = &fat32_driver_state.dir_table_buf;
    struct FAT32FileAllocationTable *fat_table = &fat32_driver_state.fat_table;

    read_clusters(&dir_table, request.parent_cluster_number, 1);
    read_clusters(&fat_table, FAT_CLUSTER_NUMBER, 1);

    int dir_idx = -1;
    for (uint32_t i = 0; i < DIRECTORY_TABLE_SIZE; i++)
    {
        if (strcmp(dir_table->table[i].name, request.name, 8) == 0 && strcmp(dir_table->table[i].ext, request.ext, 3) == 0) 
        {
            dir_idx = i;
        }
    }
    if (dir_idx == -1)
    {
        return -1;
    }
    uint32_t cluster = merge_cluster(dir_table->table[dir_idx].cluster_low, dir_table->table[dir_idx].cluster_high);
    // Folder, check if the folder is empty or not
    if (dir_table->table[dir_idx].attribute == ATTR_SUBDIRECTORY)
    {
        struct FAT32DirectoryTable folder_table;
        uint32_t i = 0;
        bool isEmpty = true;

        read_clusters(&folder_table, cluster, 1);

        while (i < DIRECTORY_TABLE_SIZE && isEmpty)
        {
            if (folder_table.table[i].user_attribute != UATTR_NOT_EMPTY)
            {
                isEmpty = false;
            }
            i++;
        }
        if (!isEmpty)
        {
            return 2;
        }
    }
    dir_table->table[dir_idx].user_attribute = 0;
    empty_string(dir_table->table[dir_idx].name, 8);
    empty_string(dir_table->table[dir_idx].ext, 3);
    dir_table->table[dir_idx].attribute = 0;
    write_clusters(&dir_table->table, request.parent_cluster_number, 1);
    
    int temp_idx;
    while (fat_table->cluster_map[cluster] != FAT32_FAT_END_OF_FILE)
    {
        temp_idx = fat_table->cluster_map[cluster];
        fat_table->cluster_map[cluster] = FAT32_FAT_EMPTY_ENTRY;
        cluster = fat_table->cluster_map[temp_idx];
    }
    fat_table->cluster_map[cluster] = FAT32_FAT_EMPTY_ENTRY;
    write_clusters(&fat_table, FAT_CLUSTER_NUMBER, 1);

    return -1;
}
