#include "mem/mem.h"
#include "kernel/sync.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

static safe_lock_t kmalloc_lock = {0};

heap_header_t *heap_free_list = NULL;

void *kmalloc(uint64_t size) {
	uint64_t flags = safe_lock(&kmalloc_lock);
	if (size == 0) {
		safe_unlock(&kmalloc_lock, flags);
		return NULL;
	}

	size = (size + 15) & ~15;

	heap_header_t* current = heap_free_list;
	heap_header_t* prev = NULL;

	while(current) {
		if (current->size >= size) {
			if (prev) {
				prev->next = current->next;
			} else {
				heap_free_list = current->next;
			}

			if (current->size >= size + sizeof(heap_header_t) + 16) {
				heap_header_t *next_block = (heap_header_t*)((uint64_t)current + sizeof(heap_header_t) + size);
				next_block->size = current->size - size - sizeof(heap_header_t);
				next_block->is_free = true;

                next_block->next = heap_free_list;
				heap_free_list = next_block;
				current->size = size;
			}
			current->is_free = false;
			safe_unlock(&kmalloc_lock, flags);
			return (void *)((uint64_t)current + sizeof(heap_header_t));
		}
		prev = current;
		current = current->next;
	}

	uint64_t old_limit = heap_current_limit;
	uint64_t allocation_size = size + sizeof(heap_header_t);
	if (heap_expand(allocation_size) < 0) {
		safe_unlock(&kmalloc_lock, flags);
		printf("\n !!! KERNEL PANIC !!!\n");
		printf("kmalloc: failed heap_expand\n");
		printf("requested: %dB",allocation_size);
		for(;;) __asm__ __volatile__("cli;hlt");
	}

	heap_header_t *new_block = (heap_header_t *)old_limit;
	new_block->size = (heap_current_limit - old_limit) - sizeof(heap_header_t);
	new_block->is_free = false;
	new_block->next = NULL;

	if (new_block->size >= size + sizeof(heap_header_t) + 16) {
		heap_header_t *extra_block = (heap_header_t *)((uint64_t) new_block + sizeof(heap_header_t) + size);
		extra_block->size = new_block->size - size - sizeof(heap_header_t);
		extra_block->is_free = true;
		extra_block->next = heap_free_list;
		heap_free_list = extra_block;
		new_block->size = size;
	}
	safe_unlock(&kmalloc_lock, flags);
	return(void *)((uint64_t)new_block + sizeof(heap_header_t));
}

void kfree(void *ptr) {
    if (!ptr) return;
    
    uint64_t flags = safe_lock(&kmalloc_lock);
    
    heap_header_t *header = (heap_header_t *)((uint64_t)ptr - sizeof(heap_header_t));
    if (header->is_free) {
        safe_unlock(&kmalloc_lock, flags);
        return; 
    }
    
    header->is_free = true;
    header->next = heap_free_list;
    heap_free_list = header;
    
    safe_unlock(&kmalloc_lock, flags);
}

void *kcalloc(size_t n, size_t size) {
	if (n != 0 && size > SIZE_MAX / n) {
		return  NULL;
	}

	size_t total_size = n * size;

	if (total_size == 0) {
		return  NULL;
	}

	void *ptr = kmalloc(total_size);
	if (!ptr) {
		return NULL;
	}

	memset(ptr, 0, total_size);

	return ptr;
}

void *krealloc(void *p, size_t new_n, size_t new_size, size_t old_total_size) {
	if (!p) {
		if (new_n != 0 && new_size > SIZE_MAX / new_n) {
			return NULL;
		}
		return kmalloc(new_n * new_size);
	}

	if (new_n == 0 || new_size == 0) {
		kfree(p);
		return NULL;
	}

	size_t new_total_size = new_n * new_size;

	void *new_ptr = kmalloc(new_total_size);
	if (!new_ptr) {
		return NULL;
	}

	size_t copy_size = (old_total_size < new_total_size) ? old_total_size : new_total_size;
	memcpy(new_ptr, p, copy_size);
	kfree(p);
	return new_ptr;
}

void *kzalloc(size_t size) {
	void *ptr = kmalloc(size);
	if (!ptr) {
		return NULL;
	}
	memset(ptr, 0, size);
	return ptr;
}