#include "memory/paging.h"
#include "memory/kmalloc.h"
#include "process/process.h"
#include <std/stdbool.h>
#include <std/stdint.h>
#include <std/string.h>

__attribute__((aligned(0x1000))
) struct PageDirectory _paging_kernel_page_directory =
		{.table = {
				 [0] =
						 {
								 .flag.present_bit = 1,
								 .flag.write_bit = 1,
								 .flag.use_pagesize_4_mb = 1,
								 .lower_address = 0,
						 },
				 // Kernel program
				 [0x300] =
						 {
								 .flag.present_bit = 1,
								 .flag.write_bit = 1,
								 .flag.use_pagesize_4_mb = 1,
								 .lower_address = 0,
						 },
				 // Kernel stack
				 [0x3FF] =
						 {
								 .flag.present_bit = 1,
								 .flag.write_bit = 1,
								 .flag.use_pagesize_4_mb = 1,
								 .lower_address = ((1 * PAGE_FRAME_SIZE) >> 22) & 0x3FF,
						 },
		 }};

static struct PageManagerState page_manager_state = {
		.mapped = {
				[0] = true,
				[1] = true
		},
		.free_page_frame_count = PAGE_FRAME_MAX_COUNT
};

void update_page_directory_entry(
		struct PageDirectory *page_dir, void *physical_addr, void *virtual_addr,
		struct PageDirectoryEntryFlag flag
) {
	uint32_t page_index = ((uint32_t)virtual_addr >> 22) & 0x3FF;
	page_dir->table[page_index].flag = flag;
	page_dir->table[page_index].lower_address =
			((uint32_t)physical_addr >> 22) & 0x3FF;
	flush_single_tlb(virtual_addr);
}

void flush_single_tlb(void *virtual_addr) {
	asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr) : "memory");
}

/* --- Memory Management --- */
bool paging_allocate_check(uint32_t amount) {
	return amount <= page_manager_state.free_page_frame_count;
}

bool paging_allocate_user_page_frame(
		struct PageDirectory *page_dir, void *virtual_addr
) {
	if (page_manager_state.free_page_frame_count == 0) return false;

	int i = 0;
	while (i < PAGE_FRAME_MAX_COUNT) {
		if (page_manager_state.mapped[i] == false) break;
		++i;
	}
	if (i == PAGE_FRAME_MAX_COUNT) return false;

	update_page_directory_entry(
			page_dir, (void *)(i * PAGE_FRAME_SIZE), virtual_addr,
			(struct PageDirectoryEntryFlag
			){.present_bit = 1, .write_bit = 1, .user = 1, .use_pagesize_4_mb = 1}
	);

	page_manager_state.mapped_address[i] = virtual_addr;
	page_manager_state.mapped_dir[i] = page_dir;
	page_manager_state.mapped[i] = true;
	page_manager_state.free_page_frame_count -= 1;
	return true;
}

bool paging_free_user_page_frame(
		struct PageDirectory *page_dir, void *virtual_addr
) {
	int i = 0;
	while (i < PAGE_FRAME_MAX_COUNT) {
		if (
				page_manager_state.mapped_address[i] == virtual_addr &&
				page_manager_state.mapped_dir[i] == page_dir
		) break;
		++i;
	}

	if (i == PAGE_FRAME_MAX_COUNT) return false;

	update_page_directory_entry(
			page_dir, (void *)0, virtual_addr,
			(struct PageDirectoryEntryFlag
			){.present_bit = 0, .write_bit = 0, .user = 0, .use_pagesize_4_mb = 0}
	);

	page_manager_state.mapped[i] = false;
	page_manager_state.free_page_frame_count += 1;
	page_manager_state.mapped_address[i] = NULL;
	page_manager_state.mapped_dir[i] = NULL;
	return true;
}

struct PageDirectory *paging_create_new_page_directory(void) {
	struct PageDirectory *dir = kmalloc_aligned(sizeof(struct PageDirectory), 0x1000);
	if (dir == NULL)
		return NULL;

	memset(dir, 0, sizeof(struct PageDirectory));
	dir->table[0x300] = (struct PageDirectoryEntry){
			.flag.present_bit = 1,
			.flag.write_bit = 1,
			.flag.use_pagesize_4_mb = 1,
			.lower_address = 0,
	};
	dir->table[0x3FF] = (struct PageDirectoryEntry){
			.flag.present_bit = 1,
			.flag.write_bit = 1,
			.flag.use_pagesize_4_mb = 1,
			.lower_address = ((1 * PAGE_FRAME_SIZE) >> 22) & 0x3FF,
	};

	return dir;
}

bool paging_free_page_directory(struct PageDirectory *page_dir) {
	for (int j = 0; j < PAGE_FRAME_MAX_COUNT; ++j) {
		struct PageDirectoryEntry *entry = &(page_dir->table[j]);
		if (entry->flag.present_bit)
			paging_free_user_page_frame(page_dir, (void *)(PAGE_FRAME_SIZE * j));
	}

	memset(page_dir, 0, sizeof(struct PageDirectory));
	kfree(page_dir);
	return false;
}

struct PageDirectory *paging_get_current_page_directory_addr(void) {
	uint32_t current_page_directory_phys_addr;
	__asm__ volatile("mov %%cr3, %0" : "=r"(current_page_directory_phys_addr) : /* <Empty> */);
	uint32_t virtual_addr_page_dir = current_page_directory_phys_addr + KERNEL_VIRTUAL_ADDRESS_BASE;
	return (struct PageDirectory *)virtual_addr_page_dir;
}

void paging_use_page_directory(struct PageDirectory *page_dir_virtual_addr) {
	uint32_t physical_addr_page_dir = (uint32_t)page_dir_virtual_addr;
	// Additional layer of check & mistake safety net
	if ((uint32_t)page_dir_virtual_addr > KERNEL_VIRTUAL_ADDRESS_BASE)
		physical_addr_page_dir -= KERNEL_VIRTUAL_ADDRESS_BASE;
	__asm__ volatile("mov %0, %%cr3" : /* <Empty> */ : "r"(physical_addr_page_dir) : "memory");
}
