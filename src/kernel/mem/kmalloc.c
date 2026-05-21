#include "mem/mem.h"
#include "kernel/sync.h"
#include "klibc/string.h"

static safe_lock_t kmalloc_lock = {0};

void *kmalloc(uint64_t size) {
	uint64_t flags = safe_lock(&kmalloc_lock);
	if (size == 0) {
		safe_unlock(&kmalloc_lock, flags);
		return NULL;
	}
	size = (size + 15) & ~15;

	/*
	if (heap_start == NULL) {
		heap_start = (heap_header_t *)heap_current_limit;
		heap_expand(PAGE_SIZE);
		heap_start->size = PAGE_SIZE - sizeof(heap_header_t);
		heap_start->is_free = true;
		heap_start->next = NULL;NULL
	}
	*/

	heap_header_t* current = heap_start;
	heap_header_t* last = NULL;

	while(current) {
		if (current->is_free && current->size >= size) {
			if (current->size >= size + sizeof(heap_header_t) + 16) {
				heap_header_t *next_block = (heap_header_t*)((uint64_t)current + sizeof(heap_header_t) + size);
				next_block->size = current->size - size - sizeof(heap_header_t);
				next_block->is_free = true;
                next_block->next = current->next;
				current->size = size;
                current->next = next_block;
			}
			current->is_free = false;
			safe_unlock(&kmalloc_lock, flags);
			return (void *)((uint64_t)current + sizeof(heap_header_t));
		}
		last = current;
		current = current->next;
	}

	uint64_t old_limit = heap_current_limit;
	uint64_t page_needed = (size + sizeof(heap_header_t) + PAGE_SIZE - 1);
	if (page_needed < 16) {
		page_needed = 16;
	}
	heap_expand(size + sizeof(heap_header_t));

	heap_header_t *new_block = (heap_header_t *)old_limit;
	new_block->size = (heap_current_limit - old_limit) - sizeof(heap_header_t);
	new_block->is_free = false;
	new_block->next = NULL;

	if (last) last->next = new_block;
	safe_unlock(&kmalloc_lock, flags);
	return(void *)((uint64_t)new_block + sizeof(heap_header_t));
}

void kfree(void *ptr) {
	if (!ptr) return;
	uint64_t flags = safe_lock(&kmalloc_lock);
	heap_header_t *header = (heap_header_t *)((uint64_t) ptr - sizeof(heap_header_t));
	header->is_free = true;
	if (header->next && header->next->is_free) {
		header->size += header->next->size + sizeof(heap_header_t);
		header->next = header->next->next;
	}
	safe_unlock(&kmalloc_lock, flags);
}