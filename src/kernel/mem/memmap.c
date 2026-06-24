#include "mem/mem.h"
#include "kernel/boot.h"
#include <stdint.h>





uint64_t memmap_get_total_pages(struct limine_memmap_response *memmap) {
	uint64_t total_pages = 0;
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *e = memmap->entries[i];
		if (e->type == LIMINE_MEMMAP_USABLE){
			total_pages += (e->length / PAGE_SIZE);
		}
	}
	return total_pages;
}