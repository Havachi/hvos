#include "mem/mem.h"
#include "kernel/sync.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>


safe_lock_t pmm_lock = { .locked = 0 };

void pmm_init(struct limine_memmap_response* memmap) {
	uint64_t max_address = 0;

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE
			|| entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
			|| entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES)
		{
			uint64_t top = entry->base + entry->length;
			if (top > max_address) max_address = top;
		}
	}

	total_pages = max_address / PAGE_SIZE;
	bitmap_size = ALIGN_UP_BOUND((total_pages + 7) / 8, PAGE_SIZE);

	uint64_t bitmap_phys_addr = 0;
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			uint64_t aligned_base = ALIGN_UP_BOUND(entry->base, PAGE_SIZE);
			if(entry->length >= bitmap_size && (aligned_base + bitmap_size) <= (entry->base + entry->length)) {
				bitmap_phys_addr = aligned_base;
				map = (uint8_t *)(bitmap_phys_addr + hhdm_offset);
				break;
			}
		}
	}


	if (!bitmap_phys_addr)	for (;;) { __asm__ __volatile("hlt"); }
	memset(map, 0xFF, bitmap_size);

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry* entry = memmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			uint64_t start = ALIGN_UP_BOUND(entry->base, PAGE_SIZE);
			uint64_t end = ALIGN_DOWN_BOUND(entry->base + entry->length, PAGE_SIZE);

			for (uint64_t phys = start; phys < end; phys += PAGE_SIZE) {
				bitmap_clear(phys / PAGE_SIZE);
			}
		}
	}

	uint64_t bitmap_start_page = bitmap_phys_addr / PAGE_SIZE;
	uint64_t bitmap_page_count = bitmap_size / PAGE_SIZE;
	for (uint64_t p = 0; p < bitmap_page_count; p++) {
		bitmap_set(bitmap_start_page + p);
	}
}

void pmm_reclaim_bootloader(struct limine_memmap_response* memmap) {
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		if(entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			uint64_t start = ALIGN_UP_BOUND(entry->base, PAGE_SIZE);
			uint64_t end = ALIGN_DOWN_BOUND(entry->base + entry->length, PAGE_SIZE);

			for (uint64_t phys = start; phys < end; phys += PAGE_SIZE) {
				bitmap_clear(phys / PAGE_SIZE);
			}
		}
	}
}

static uint64_t pmm_find_free_range(uint64_t start_page, uint64_t end_page, uint64_t n) {
	if (end_page < start_page || (end_page - start_page) < n) return 0;
	for (uint64_t i = start_page ; i <= end_page - n; i++) {
		if (!bitmap_test(i)) {
			uint64_t found = 1;
			uint64_t j;
			for (j = 1; j < n; j++) {
				if (bitmap_test(i + j)) {
					found = 0;
					break;
				}
 			}
			if (found) {
				return i;
			} else {
				i += j;
			}
		}
	}
	return 0;
}

void *pmm_alloc(void) {
    uint64_t flags = safe_lock(&pmm_lock);
    // Never allocate frames 0 to 511 (< 2 MiB)
    for (uint64_t i = PMM_FIRST_FREE_FRAME; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            safe_unlock(&pmm_lock, flags);
            return (void *)(i * PAGE_SIZE);
        }
    }
    safe_unlock(&pmm_lock, flags);
    return NULL;
}

void *pmm_alloc_n(uint64_t n) {
	if(n == 0) return NULL;
	if (n == 1) return pmm_alloc();
	uint64_t flags = safe_lock(&pmm_lock);
	uint64_t start_page = 0;
	if(total_pages > MAX_DMA32_PAGE) {
		start_page = pmm_find_free_range(MAX_DMA32_PAGE, total_pages, n);
	}

	if (start_page == 0) {
		uint64_t limit = (total_pages < MAX_DMA32_PAGE) ? total_pages : MAX_DMA32_PAGE;
		start_page = pmm_find_free_range(PMM_FIRST_FREE_FRAME, limit, n);
	}

	if (start_page != 0) {
		for (uint64_t k = 0; k < n; k++) {
			bitmap_set(start_page + k);
		}
		used_pages += n;
		safe_unlock(&pmm_lock, flags);
		return (void *)(start_page * PAGE_SIZE);
	}
	safe_unlock(&pmm_lock, flags);
	return NULL;
}

void pmm_free(void* addr) {
	uint64_t flags = safe_lock(&pmm_lock);
	uint64_t page_index = (uint64_t)addr / PAGE_SIZE;
	if (bitmap_test(page_index)) {
		bitmap_clear(page_index);
		used_pages--;
	}
	safe_unlock(&pmm_lock, flags);
}

void *pmm_alloc_n_dma32(uint64_t n) {
    if (n == 0) return NULL;

    uint64_t flags = safe_lock(&pmm_lock);
    uint64_t limit = (total_pages < MAX_DMA32_PAGE) ? total_pages : MAX_DMA32_PAGE;
	uint64_t start_page = pmm_find_free_range(1, limit, n);
	if (start_page != 0) {
		for (uint64_t k = 0; k < n; k++) {
			bitmap_set(start_page + k);
		}
		used_pages += n;
		safe_unlock(&pmm_lock, flags);
		return (void *)(start_page * PAGE_SIZE);
	}
	safe_unlock(&pmm_lock, flags);
	return NULL;
}

void *pmm_alloc_dma32(void) {
    return pmm_alloc_n_dma32(1);
}