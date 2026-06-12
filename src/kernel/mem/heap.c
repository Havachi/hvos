#include <stdio.h>
#include "mem/mem.h"
#include <string.h>
#include <stdint.h>

#define HEAP_INIT_PAGES 128


void heap_init(void) {
    heap_start = (heap_header_t *)heap_current_limit;
    heap_expand(HEAP_INIT_PAGES * PAGE_SIZE);
	
    heap_start->size = (HEAP_INIT_PAGES * PAGE_SIZE) - sizeof(heap_header_t);
    heap_start->is_free = true;
    heap_start->next = NULL;
}

void heap_expand(uint64_t size_needed) {
	uint64_t pages = (size_needed + sizeof(heap_header_t) + PAGE_SIZE - 1) / PAGE_SIZE;
	for (uint64_t i = 0; i < pages; i++) {
		uint64_t phys = (uint64_t)pmm_alloc();
		if (!phys) {
			printf("KERNEL PANIC: heap_expand: OOM\n");
			for (;;) asm volatile("hlt");
		}
		map_page(kernel_pml4, heap_current_limit, phys, PTE_PRESENT | PTE_WRITABLE);
		heap_current_limit += PAGE_SIZE;
	}
}