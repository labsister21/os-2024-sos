#include "memory/kmalloc.h"
#include "driver/tty.h"
#include "kernel-entrypoint.h"
#include "text/framebuffer.h"
#include <std/stdbool.h>
#include <std/stddef.h>

#define SMALLEST_BLOCK 0x100
#define BIT_IN_BYTE 8

// All struct local here
struct Heap {
	void *start;
	void *end;
};

static struct Heap heap = {
		.start = (void *)&_linker_kernel_virtual_addr_end,
		.end = (void *)&_linker_kernel_break_address,
};

struct Block {
	uint32_t size;
	bool used;
	struct Block *prev;
	struct Block *next;
	void *data;
};

static struct Block *start_block = NULL;

void *kmalloc_aligned(uint32_t size, uint32_t align) {
	if (start_block == NULL) {
		int heap_size = (heap.end - heap.start) / BIT_IN_BYTE;
		start_block = heap.start;
		start_block->size = heap_size - sizeof(struct Block);
		start_block->used = false;
		start_block->next = NULL;
		start_block->prev = NULL;
		start_block->data = (char *)start_block + sizeof(struct Block);
	}

	(void)align;

	void *result = NULL;
	struct Block *current_block = start_block;
	while (current_block != NULL) {
		uint32_t base_data = (uint32_t)((char *)current_block + sizeof(struct Block));
		uint32_t align_padding = base_data % align;
		if (align_padding != 0) align_padding = align - align_padding;
		uint32_t aligned_size = size + align_padding;

		if (aligned_size < SMALLEST_BLOCK)
			aligned_size = SMALLEST_BLOCK;

		if (!current_block->used && current_block->size >= aligned_size) {
			uint32_t unused_size = current_block->size - aligned_size;
			if (unused_size > (sizeof(struct Block) + SMALLEST_BLOCK)) { // Creating new block
				struct Block *new_block = (void *)(((char *)current_block) + sizeof(struct Block) + aligned_size);
				new_block->size = unused_size - sizeof(struct Block);
				new_block->used = false;
				new_block->next = current_block->next;
				new_block->prev = current_block;
				new_block->data = (char *)new_block + sizeof(struct Block);

				current_block->next = new_block;
				current_block->size = aligned_size;
			}

			current_block->used = true;
			result = (void *)((char *)current_block + sizeof(struct Block) + align_padding);
			current_block->data = result;
			break;
		}

		current_block = current_block->next;
	}

	return result;
};

void *kmalloc(uint32_t size) {
	return kmalloc_aligned(size, 4);
};

static void combine_block(struct Block *prev, struct Block *next) {
	prev->size += sizeof(struct Block) + next->size;
	prev->next = next->next;
	if (next->next != NULL)
		next->next->prev = prev;
}

void kfree(void *address) {
	if (start_block == NULL) return;

	struct Block *current_block = start_block;
	while (current_block != NULL) {
		if (current_block->used && current_block->data == address) {
			current_block->used = false;

			if (current_block->next != NULL && !current_block->next->used)
				combine_block(current_block, current_block->next);

			if (current_block->prev != NULL && !current_block->prev->used)
				combine_block(current_block->prev, current_block);

			break;
		}

		current_block = current_block->next;
	}
};

// void ktest() {
// 	struct Block *current_block = start_block;
// 	while (current_block != NULL) {
// 		fputc('\n');
//
// 		framebuffer_put_hex((uint32_t)current_block->used);
// 		fputc(' ');
// 		framebuffer_put_hex((uint32_t)current_block->size);
// 		fputc(' ');
// 		framebuffer_put_hex((uint32_t)current_block->data);
// 		fputc(' ');
// 		framebuffer_put_hex((uint32_t)current_block->prev);
// 		fputc(' ');
// 		framebuffer_put_hex((uint32_t)current_block->next);
// 		fputc(' ');
//
// 		current_block = current_block->next;
// 	}
// }
