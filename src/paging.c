#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "header/memory/paging.h"

__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        [0] = {
            .flag.present_bit       = 1,
            .flag.read_write        = 1,
            .lower_address          = 0,
            .flag.page_size         = 1,
        },
        [0x300] = {
            .flag.present_bit       = 1,
            .flag.read_write        = 1,
            .lower_address          = 0,
            .flag.page_size         = 1,
        },
    }
};

static struct PageManagerState page_manager_state = {
    .page_frame_map = {
        [0]                            = true,
        [1 ... PAGE_FRAME_MAX_COUNT-1] = false
    },
    // TODO: Initialize page manager state properly
    .free_page_frame_count = PAGE_FRAME_MAX_COUNT-1
};

void update_page_directory_entry(
    struct PageDirectory *page_dir,
    void *physical_addr, 
    void *virtual_addr, 
    struct PageDirectoryEntryFlag flag
) {
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;
    page_dir->table[page_index].flag          = flag;
    page_dir->table[page_index].lower_address = ((uint32_t) physical_addr >> 22) & 0x3FF;
    flush_single_tlb(virtual_addr);
}

void flush_single_tlb(void *virtual_addr) {
    asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr): "memory");
}



/* --- Memory Management --- */
// TODO: Implement
bool paging_allocate_check(uint32_t amount) {
    // TODO: Check whether requested amount is available
    uint32_t needed_memory = (amount + PAGE_FRAME_SIZE - 1)/PAGE_FRAME_SIZE;
    return page_manager_state.free_page_frame_count >= needed_memory;
}


bool paging_allocate_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    /**
     * TODO: Find free physical frame and map virtual frame into it
     * - Find free physical frame in page_manager_state.page_frame_map[] using any strategies
     * - Mark page_manager_state.page_frame_map[]
     * - Update page directory with user flags:
     *     > present bit    true
     *     > write bit      true
     *     > user bit       true
     *     > pagesize 4 mb  true
     */ 
    // Memory full
    if (page_manager_state.free_page_frame_count == 0)
    {
        return 0;
    }
    
    int i = 0;
    bool found = false;
    while (i < PAGE_FRAME_MAX_COUNT && !found)
    {
        if (!page_manager_state.page_frame_map[i])
        {
            found = true;
        }
    }
    struct PageDirectoryEntryFlag entry_flag = {
        .present_bit = 1,
        .read_write = 1,
        .user_supervisor = 1,
        .page_size = 1
    };
    update_page_directory_entry(page_dir,(void *)(i * PAGE_FRAME_SIZE) , virtual_addr, entry_flag);
    page_manager_state.page_frame_map[i] = true;
    page_manager_state.free_page_frame_count -= 1;   
    page_manager_state.mapped_address[i] = virtual_addr; 
    return true;
}

bool paging_free_user_page_frame(struct PageDirectory *page_dir, void *virtual_addr) {
    /* 
     * TODO: Deallocate a physical frame from respective virtual address
     * - Use the page_dir.table values to check mapped physical frame
     * - Remove the entry by setting it into 0
     */
    int i = 0;
    while (i < PAGE_FRAME_MAX_COUNT)
    {
        if (page_manager_state.mapped_address[i] == virtual_addr)
        {
            break;
        }
    }
    struct PageDirectoryEntryFlag entry_flag = {
        .present_bit = 0,
        .read_write = 0,
        .user_supervisor = 0,
        .page_size = 0
    };
    // is the physical address correct?
    // uhh i can't track the index
    update_page_directory_entry(page_dir, (void *)(i * PAGE_FRAME_SIZE), virtual_addr, entry_flag);
    page_manager_state.page_frame_map[i] = false;
    page_manager_state.free_page_frame_count += 1;
    return true;
}

