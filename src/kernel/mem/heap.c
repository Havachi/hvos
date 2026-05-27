#include "mem/mem.h"
#include "klibc/string.h"

extern pml4_table_t *kernel_pml4;

void heap_init(volatile pt_entry *pml4) {
	current_pml4 = pml4;
	heap_start = (heap_header_t *)heap_current_limit;
	uint64_t initial_pages = 128;
	heap_expand(PAGE_SIZE);

	heap_start->size = (initial_pages * PAGE_SIZE) - sizeof(heap_header_t);
	heap_start->is_free = true;
	heap_start->next = NULL;
}

void heap_expand(uint64_t size_needed) {
	uint64_t pages = (size_needed + sizeof(heap_header_t) + PAGE_SIZE -1);
	for (uint64_t i = 0; i < pages; i++) {
		void *phys = pmm_alloc();
		map_page(kernel_pml4, heap_current_limit, (uint64_t)phys, PTE_PRESENT | PTE_WRITABLE);
		heap_current_limit += PAGE_SIZE;
	}
}