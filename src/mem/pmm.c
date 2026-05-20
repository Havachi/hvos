#include "kernel/mem.h"

safe_lock_t pmm_lock = { .locked = 0 };

void pmm_init(struct limine_memmap_response* memmap) {
	uint64_t max_address = 0;

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE
			|| entry->type == LIMINE_MEMMAP_FRAMEBUFFER
			|| entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
			|| entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES)
		{
			uint64_t end_addr = entry->base + entry->length;
			if (end_addr > max_address) {
				max_address = end_addr;
			}
		}
	}

	total_pages = max_address / PAGE_SIZE;

	uint64_t raw_bitmap_size = (total_pages + 7) / 8;
	bitmap_size = (raw_bitmap_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	uint64_t bitmap_phys_addr = 0;

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *entry = memmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
			bitmap_phys_addr = entry->base;
			bitmap = (uint8_t *)(bitmap_phys_addr + hhdm_offset);
			break;
		}
	}
	if (bitmap_phys_addr == 0){
		for (;;) { asm volatile("hlt"); }
	}
	memset(bitmap, 0xFF, bitmap_size);

	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry* entry = memmap->entries[i];
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE) {
				bitmap_clear((entry->base + j) / PAGE_SIZE);
			}
		}
	}

	uint64_t bitmap_start_page = bitmap_phys_addr / PAGE_SIZE;
	uint64_t bitmap_page_count = bitmap_size / PAGE_SIZE;
	for (uint64_t p = 0; p < bitmap_page_count; p++) {
		bitmap_set(bitmap_start_page + p);
	}
}

void *pmm_alloc() {
	uint64_t flags = safe_lock(&pmm_lock);
	for (uint64_t i = 0; i < total_pages; i++) {
		if (!bitmap_test(i)) {
			bitmap_set(i);
			safe_unlock(&pmm_lock, flags);
			return (void*) (i * PAGE_SIZE);
		}
	}
	safe_unlock(&pmm_lock, flags);
	return NULL;
}

void pmm_free(void* addr) {
	uint64_t flags = safe_lock(&pmm_lock);
	uint64_t page_index = (uint64_t)addr / PAGE_SIZE;
	bitmap_clear(page_index);
	safe_unlock(&pmm_lock, flags);
}