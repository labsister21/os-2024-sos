#include "memory/kmalloc.h"
#include "driver/tty.h"
#include "kernel-entrypoint.h"
#include "text/framebuffer.h"
#include <std/stdbool.h>
#include <std/stddef.h>

#define SMALLEST_BLOCK 0x100

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
};

static struct Block *start_block = NULL;

void *kmalloc(uint32_t size) {
	if (start_block == NULL) {
		int heap_size = (heap.end - heap.start) / 8; // Convert size from bit to byte
		start_block = heap.start;
		start_block->size = heap_size - sizeof(struct Block);
		start_block->used = false;
		start_block->next = NULL;
		start_block->prev = NULL;
	}

	if (size < SMALLEST_BLOCK)
		size = SMALLEST_BLOCK;

	void *result = NULL;
	struct Block *current_block = start_block;
	while (current_block != NULL) {
		if (!current_block->used && current_block->size >= size) {
			uint32_t unused_size = current_block->size - size;
			if (unused_size > (sizeof(struct Block) + SMALLEST_BLOCK)) { // Creating new block
				struct Block *new_block = (void *)(((char *)current_block) + sizeof(struct Block) + size);
				new_block->size = unused_size - sizeof(struct Block);
				new_block->used = false;
				new_block->next = current_block->next;
				new_block->prev = current_block;

				current_block->next = new_block;
				current_block->size = size;
			}

			current_block->used = true;
			result = (void *)((char *)current_block + sizeof(struct Block));
			break;
		}

		current_block = current_block->next;
	}

	return result;
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
	fputc('\n');
	while (current_block != NULL) {
		framebuffer_put_hex((uint32_t)current_block);
		fputc(' ');
		void *current_address = (char *)current_block + sizeof(struct Block);
		if (current_block->used && current_address == address) {
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
