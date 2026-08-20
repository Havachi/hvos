#include "mem/mem.h"
#include "kernel/sync.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

static safe_lock_t kmalloc_lock = {0};

heap_header_t *heap_free_list = NULL;

#define TRACE_COUNT 64
static alloc_trace_t g_alloc_trace[TRACE_COUNT];
static size_t g_alloc_idx = 0;

static int heap_header_in_bounds(const heap_header_t *hdr) {
	uintptr_t addr = (uintptr_t)hdr;
	return addr >= (uintptr_t)heap_start
		&& addr + sizeof(heap_header_t) <= heap_current_limit;
}

static int heap_is_block_start(heap_header_t *hdr) {
	if (!heap_header_in_bounds(hdr))
		return 0;
	uintptr_t scan = (uintptr_t)heap_start;
	while (scan < heap_current_limit) {
		if (scan == (uintptr_t)hdr)
			return 1;
		heap_header_t *b = (heap_header_t *)scan;
		if (b->size == 0
			|| scan + sizeof(heap_header_t) + b->size + 8 > heap_current_limit)
			return 0;
		scan += sizeof(heap_header_t) + b->size + 8;
	}
	return 0;
}

static void heap_panic_corrupt_list(const char *where, heap_header_t *curr) {
	printf("\n[KFREE PANIC] Corrupt heap free list at %s\n", where);
	printf("  curr:      %p  (size=%lu, is_free=%lu)\n",
	       curr, curr ? curr->size : 0, curr ? curr->is_free : 0);
	printf("  curr->next:%p\n", curr ? curr->next : NULL);
	printf("  heap:      [%p, %p)\n", heap_start, (void *)heap_current_limit);
	printf("Caller: %p\n", __builtin_return_address(0));
	for (;;) __asm__ volatile("cli; hlt");
}

static void heap_insert_free_block(heap_header_t *block) {
	if (!heap_free_list || block < heap_free_list) {
		block->next = heap_free_list;
		heap_free_list = block;
	} else {
		heap_header_t *curr = heap_free_list;
		while (curr->next && curr->next < block) {
			if (!heap_header_in_bounds(curr) || !heap_is_block_start(curr))
				heap_panic_corrupt_list("insert walk (curr)", curr);
			if (curr->next && !heap_is_block_start(curr->next))
				heap_panic_corrupt_list("insert walk (next)", curr);
			curr = curr->next;
		}
		block->next = curr->next;
		curr->next = block;
	}
}

static void heap_coalesce_all(void) {
	heap_header_t *curr = heap_free_list;
	while (curr && curr->next) {
		if (!heap_header_in_bounds(curr) || !heap_is_block_start(curr))
			heap_panic_corrupt_list("coalesce (curr)", curr);
		if (!heap_is_block_start(curr->next))
			heap_panic_corrupt_list("coalesce (next)", curr);
		if (!curr->next->is_free)
			heap_panic_corrupt_list("coalesce (next in use)", curr);

		uintptr_t curr_end = (uintptr_t)curr + sizeof(heap_header_t) + curr->size + 8;
		if (curr_end == (uintptr_t)curr->next) {
			curr->size += sizeof(heap_header_t) + curr->next->size + 8;
			curr->next = curr->next->next;
			uintptr_t merged_canary = (uintptr_t)curr + sizeof(heap_header_t) + curr->size;
			if (merged_canary + 8 > heap_current_limit) {
				heap_panic_corrupt_list("coalesce (merged canary OOB)", curr);
			}
			*(uint64_t *)merged_canary = CANARY_MAGIC;
		} else {
			curr = curr->next;
		}
	}
}

static void record_trace(void *ptr, uint64_t size, const char *file, int line) {
	if (!ptr) return;
	size_t idx = g_alloc_idx % TRACE_COUNT;
	g_alloc_trace[idx].ptr = ptr;
	g_alloc_trace[idx].size = size;
	g_alloc_trace[idx].file = file;
	g_alloc_trace[idx].line = line;
	g_alloc_idx++;
}

void *debug_kmalloc(uint64_t size, const char *file, int line) {
	void *ptr = _kmalloc(size);
	if (ptr) {
		uint64_t flags = safe_lock(&kmalloc_lock);
		record_trace(ptr, size, file, line);
		safe_unlock(&kmalloc_lock, flags);
	}
	return ptr;
}

void *debug_kzalloc(uint64_t size, const char *file, int line) {
	void *ptr = _kzalloc(size);
	if (ptr) {
		uint64_t flags = safe_lock(&kmalloc_lock);
		record_trace(ptr, size, file, line);
		safe_unlock(&kmalloc_lock, flags);
	}
	return ptr;
}

void *debug_kcalloc(uint64_t n, uint64_t size, const char *file, int line) {
	void *ptr = _kcalloc(n, size);
	if (ptr) {
		uint64_t flags = safe_lock(&kmalloc_lock);
		record_trace(ptr, n * size, file, line);
		safe_unlock(&kmalloc_lock, flags);
	}
	return ptr;
}

void *_kmalloc(uint64_t size) {
	if (size == 0) return NULL;

	uint64_t flags = safe_lock(&kmalloc_lock);
	size = ALIGN_UP_BOUND(size, 16);

	heap_header_t *current = heap_free_list;
	heap_header_t *prev = NULL;

	// 1. First-Fit Search
	while (current) {
		if (current->size >= size) {
			size_t min_split_size = size + sizeof(heap_header_t) + 8 + 32;

			if (current->size >= min_split_size) {
				// Split block
				uintptr_t canary_addr = (uintptr_t)current + sizeof(heap_header_t) + size;
				*(uint64_t *)canary_addr = CANARY_MAGIC;

				uintptr_t remainder_addr = canary_addr + 8;
				heap_header_t *remainder = (heap_header_t *)remainder_addr;
				
				remainder->size = current->size - size - sizeof(heap_header_t) - 8;
				remainder->is_free = 1;
				remainder->next = current->next;

				// Write canary for the new remainder block so it is valid when freed/coalesced
				uintptr_t rem_canary = (uintptr_t)remainder + sizeof(heap_header_t) + remainder->size;
				*(uint64_t *)rem_canary = CANARY_MAGIC;

				if (prev) prev->next = remainder;
				else heap_free_list = remainder;

				current->size = size;
			} else {
				// Take full block without splitting
				if (prev) prev->next = current->next;
				else heap_free_list = current->next;

				uintptr_t canary_addr = (uintptr_t)current + sizeof(heap_header_t) + current->size;
				*(uint64_t *)canary_addr = CANARY_MAGIC;
			}

			current->is_free = 0;
			current->next = NULL;

			safe_unlock(&kmalloc_lock, flags);
			return (void *)((uintptr_t)current + sizeof(heap_header_t));
		}

		prev = current;
		if (current->next && !heap_is_block_start(current->next))
			heap_panic_corrupt_list("kmalloc search", current);
		current = current->next;
	}

	// 2. Expand Heap on Miss
	uint64_t old_limit = heap_current_limit;
	uint64_t alloc_bytes = size + sizeof(heap_header_t) + 8;

	if (heap_expand(alloc_bytes) < 0) {
		safe_unlock(&kmalloc_lock, flags);
		return NULL;
	}

	heap_header_t *new_block = (heap_header_t *)old_limit;
	new_block->is_free = 0;
	new_block->next = NULL;

	uintptr_t target_canary = (uintptr_t)new_block + sizeof(heap_header_t) + size;
	uintptr_t extra_start = target_canary + 8;

	// Check if trailing heap expansion space can form a new free block
	if (extra_start + sizeof(heap_header_t) + 32 <= heap_current_limit) {
		new_block->size = size;
		*(uint64_t *)target_canary = CANARY_MAGIC;

		heap_header_t *extra = (heap_header_t *)extra_start;
		extra->size = heap_current_limit - extra_start - sizeof(heap_header_t) - 8;
		extra->next = NULL;

		// CRITICAL: Initialize canary for 'extra' before handing to kfree()
		uintptr_t extra_canary = (uintptr_t)extra + sizeof(heap_header_t) + extra->size;
		*(uint64_t *)extra_canary = CANARY_MAGIC;

		extra->is_free = 1;
		extra->next = NULL;
		heap_insert_free_block(extra);
		heap_coalesce_all();

		safe_unlock(&kmalloc_lock, flags);
		return (void *)((uintptr_t)new_block + sizeof(heap_header_t));
	} else {
		// Trailing space too small for a new header + payload; absorb entirely into new_block
		new_block->size = (heap_current_limit - old_limit) - sizeof(heap_header_t) - 8;
		uintptr_t absorbed_canary = (uintptr_t)new_block + sizeof(heap_header_t) + new_block->size;
		*(uint64_t *)absorbed_canary = CANARY_MAGIC;

		safe_unlock(&kmalloc_lock, flags);
		return (void *)((uintptr_t)new_block + sizeof(heap_header_t));
	}
}

void kfree(void *ptr) {
	if (!ptr) return;

	uint64_t flags = safe_lock(&kmalloc_lock);
	uintptr_t vaddr = (uintptr_t)ptr;

	if (vaddr < KERNEL_HEAP_START || vaddr >= heap_current_limit) {
		safe_unlock(&kmalloc_lock, flags);
		printf("\n[KFREE PANIC] Invalid pointer free (%p): Outside heap bounds!\n", ptr);
		printf("Caller: %p\n", __builtin_return_address(0));
		for (;;) __asm__ volatile("cli; hlt");
	}

	heap_header_t *block = (heap_header_t *)(vaddr - sizeof(heap_header_t));

	if (block->is_free) {
		safe_unlock(&kmalloc_lock, flags);
		printf("\n[KFREE PANIC] Double free detected at %p!\n", ptr);
		printf("Caller: %p\n", __builtin_return_address(0));
		for (;;) __asm__ volatile("cli; hlt");
	}

	if (!heap_header_in_bounds(block)) {
		safe_unlock(&kmalloc_lock, flags);
		printf("\n[KFREE PANIC] Header out of bounds at %p!\n", block);
		printf("Caller: %p\n", __builtin_return_address(0));
		for (;;) __asm__ volatile("cli; hlt");
	}

	if (block->size == 0 || (vaddr + block->size) > heap_current_limit) {
		safe_unlock(&kmalloc_lock, flags);
		printf("\n[KFREE PANIC] Corrupted block header at %p (size: %lu)\n", ptr, block->size);
		printf("Caller: %p\n", __builtin_return_address(0));
		for (;;) __asm__ volatile("cli; hlt");
	}

	uint64_t *canary = (uint64_t *)(vaddr + block->size);
	if (*canary != CANARY_MAGIC) {
		safe_unlock(&kmalloc_lock, flags);
		printf("\n[KFREE PANIC] Buffer overflow at %p! Canary: 0x%lx (Expected: 0x%lx)\n",
		       ptr, *canary, CANARY_MAGIC);
		printf("Caller: %p\n", __builtin_return_address(0));
		for (;;) __asm__ volatile("cli; hlt");
	}

	block->is_free = 1;
	heap_insert_free_block(block);
	heap_coalesce_all();

	safe_unlock(&kmalloc_lock, flags);
}

void *_kcalloc(size_t n, size_t size) {
	if (n != 0 && size > SIZE_MAX / n) return NULL;
	size_t total_size = n * size;
	if (total_size == 0) return NULL;

	void *ptr = _kmalloc(total_size);
	if (ptr) memset(ptr, 0, total_size);
	return ptr;
}

void *krealloc(void *p, size_t new_n, size_t new_size, size_t old_total_size) {
	if (!p) {
		if (new_n != 0 && new_size > SIZE_MAX / new_n) return NULL;
		return _kmalloc(new_n * new_size);
	}

	if (new_n == 0 || new_size == 0) {
		kfree(p);
		return NULL;
	}

	size_t new_total_size = new_n * new_size;
	void *new_ptr = _kmalloc(new_total_size);
	if (!new_ptr) return NULL;

	size_t copy_size = (old_total_size < new_total_size) ? old_total_size : new_total_size;
	memcpy(new_ptr, p, copy_size);
	kfree(p);
	return new_ptr;
}

void *_kzalloc(size_t size) {
	void *ptr = _kmalloc(size);
	if (ptr) memset(ptr, 0, size);
	return ptr;
}

void mem_dump_alloc_trace(void) {
	printf("\n=== KMALLOC ALLOC TRACE (last %d) ===\n", TRACE_COUNT);
	size_t count = g_alloc_idx < TRACE_COUNT ? g_alloc_idx : TRACE_COUNT;
	for (size_t i = 0; i < count; i++) {
		size_t idx = (g_alloc_idx - 1 - i) % TRACE_COUNT;
		alloc_trace_t *t = &g_alloc_trace[idx];
		if (t->ptr)
			printf("  %p  %8lu B  %s:%d\n", t->ptr, t->size, t->file, t->line);
	}
}

void mem_verify_heap(void) {
	uint64_t flags = safe_lock(&kmalloc_lock);
	uint64_t free_bytes = 0;
	uint64_t used_bytes = 0;
	uint64_t free_blocks = 0;
	uint64_t used_blocks = 0;

	for (heap_header_t *curr = heap_free_list; curr; curr = curr->next) {
		if (!curr->is_free) {
			printf("[HEAP VERIFY] free list node marked in-use at %p\n", curr);
		}
		uintptr_t canary = (uintptr_t)curr + sizeof(heap_header_t) + curr->size;
		if (canary + 8 > heap_current_limit) {
			printf("[HEAP VERIFY] canary OOB for free block at %p (size %lu)\n",
			       curr, curr->size);
		} else if (*(uint64_t *)canary != CANARY_MAGIC) {
			printf("[HEAP VERIFY] corrupted canary on free block at %p "
			       "(got 0x%lx)\n", curr, *(uint64_t *)canary);
		}
		free_bytes += curr->size;
		free_blocks++;
	}

	uintptr_t scan = (uintptr_t)heap_start;
	while (scan + sizeof(heap_header_t) <= heap_current_limit) {
		heap_header_t *block = (heap_header_t *)scan;
		if (block->size == 0 || scan + sizeof(heap_header_t) + block->size + 8 > heap_current_limit) {
			printf("[HEAP VERIFY] corrupt header at %p (size %lu)\n", block, block->size);
			break;
		}
		uint64_t *canary = (uint64_t *)(scan + sizeof(heap_header_t) + block->size);
		if (*canary != CANARY_MAGIC) {
			printf("[HEAP VERIFY] bad canary at %p (is_free=%d, size=%lu, magic=0x%lx)\n",
			       block, block->is_free, block->size, *canary);
		}
		if (!block->is_free) {
			used_bytes += block->size;
			used_blocks++;
		}
		scan += sizeof(heap_header_t) + block->size + 8;
	}

	printf("[HEAP VERIFY] limit=%p  free=%lu B (%lu blk)  used=%lu B (%lu blk)\n",
	       (void *)heap_current_limit, free_bytes, free_blocks, used_bytes, used_blocks);
	safe_unlock(&kmalloc_lock, flags);
}

void mem_dump_pmm_stats(void) {
	printf("[PMM] total_pages=%lu  used_pages=%lu  free_pages=%lu\n",
	       total_pages, used_pages, total_pages - used_pages);
}

void *kmalloc_dma32(size_t size) {
	if (size == 0) return NULL;
	
	size_t total_size = size + sizeof(dma32_header_t);
	size_t pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
	
	uintptr_t phys_addr = (uintptr_t)pmm_alloc_n_dma32(pages);
	if (!phys_addr) return NULL;

	void *virt_addr = (void *)PHYS_TO_VIRT(phys_addr);
	dma32_header_t *hdr = (dma32_header_t *)virt_addr;
	hdr->page_count = pages;
	hdr->requested_size = size;
	return (void *)((uintptr_t)virt_addr + sizeof(dma32_header_t));
}

void kfree_dma32(void *ptr) {
	if (!ptr) return;

	dma32_header_t *hdr = (dma32_header_t *)((uintptr_t)ptr - sizeof(dma32_header_t));
	uintptr_t virt_start = (uintptr_t)hdr;
	uintptr_t phys_start = (uintptr_t)VIRT_TO_PHYS(virt_start);
	for (uint64_t i = 0; i < hdr->page_count; i++) {
		pmm_free((void *)(phys_start + (i * PAGE_SIZE)));
	}
}