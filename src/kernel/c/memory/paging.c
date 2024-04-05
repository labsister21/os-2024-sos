#include "memory/paging.h"
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
				 [0x300] =
						 {
								 .flag.present_bit = 1,
								 .flag.write_bit = 1,
								 .flag.use_pagesize_4_mb = 1,
								 .lower_address = 0,
						 },
		 }};

static struct PageManagerState page_manager_state = {
		.mapped = {[0 ... PAGE_FRAME_MAX_COUNT - 1] = 0},
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
	uint32_t needed = (amount + PAGE_FRAME_SIZE - 1) / PAGE_FRAME_SIZE;
	return needed <= page_manager_state.free_page_frame_count;
}

bool paging_allocate_user_page_frame(
		struct PageDirectory *page_dir, void *virtual_addr
) {
	if (page_manager_state.free_page_frame_count == 0) return false;

	int i = 0;
	while (i < PAGE_ENTRY_COUNT) {
		if (page_manager_state.mapped[i] == false) break;
		++i;
	}

	update_page_directory_entry(
			page_dir, (void *)(i * PAGE_FRAME_SIZE), virtual_addr,
			(struct PageDirectoryEntryFlag
			){.present_bit = 1, .write_bit = 1, .user = 1, .use_pagesize_4_mb = 1}
	);

	page_manager_state.mapped_address[i] = virtual_addr;
	page_manager_state.mapped[i] = true;
	page_manager_state.free_page_frame_count -= 1;
	return true;
}

bool paging_free_user_page_frame(
		struct PageDirectory *page_dir, void *virtual_addr
) {
	int i = 0;
	while (i < PAGE_ENTRY_COUNT) {
		if (page_manager_state.mapped_address[i] == virtual_addr) break;
	}

	update_page_directory_entry(
			page_dir, (void *)0, virtual_addr,
			(struct PageDirectoryEntryFlag
			){.present_bit = 0, .write_bit = 0, .user = 0, .use_pagesize_4_mb = 0}
	);

	page_manager_state.mapped[i] = false;
	page_manager_state.free_page_frame_count += 1;
	return true;
}
