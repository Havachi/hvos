#include <stdio.h>
#include "mem/mem.h"
#include <string.h>
#include <stdint.h>

#define HEAP_INIT_PAGES 1024

extern heap_header_t *heap_free_list;


void heap_init(void) {
	heap_current_limit = KERNEL_HEAP_START;
    heap_start = (heap_header_t *)heap_current_limit;

	heap_free_list = NULL;
	uint64_t total_bytes_needed = HEAP_INIT_PAGES * PAGE_SIZE;
	heap_expand(total_bytes_needed);
	heap_header_t *init_block = heap_start;
	init_block->size = (heap_current_limit - (uint64_t)heap_start) - sizeof(heap_header_t);
	init_block->is_free = true;
	init_block->next = NULL;
	heap_free_list = init_block;
	printf("head size: %d pages\n", HEAP_INIT_PAGES);
}

int heap_expand(uint64_t size_needed) {
	uint64_t pages = (size_needed + sizeof(heap_header_t) + PAGE_SIZE - 1) / PAGE_SIZE;
	for (uint64_t i = 0; i < pages; i++) {
		uint64_t phys = (uint64_t)pmm_alloc();
		if (!phys) {
			return -1;
		}
		map_page(kernel_pml4, heap_current_limit, phys, PTE_PRESENT | PTE_WRITABLE);
		heap_current_limit += PAGE_SIZE;
	}
	return 0;
}