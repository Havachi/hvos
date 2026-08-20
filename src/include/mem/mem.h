#ifndef HVOS_MEM_H
#define HVOS_MEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "limine.h"
#include "mem/paging.h"


#define PAGE_SIZE			4096
#define PTE_PRESENT			(1ULL << 0)
#define PTE_WRITABLE		(1ULL << 1)
#define PTE_USER			(1ULL << 2)
#define PTE_PWT				(1ULL << 3)
#define PTE_PCD				(1ULL << 4)
#define PTE_PSE				(1ULL << 7)
#define PTE_NX				(1ULL << 63)
#define PTE_ADDR_MASK		0x000ffffffffff000
#define USR_STACK_BASE		0x00007FFFFFFFF000
#define KERNEL_HEAP_START 	0xffffa00000000000
#define USR_HEAP_BASE		0x10000000
#define PHYS_TO_VIRT(phys)	((uint64_t)(phys) + hhdm_offset)
#define VIRT_TO_PHYS(virt)	(((uint64_t)(virt) != 0) ? ((uint64_t)(virt) - hhdm_offset) : (uint64_t)(virt))
#define KERNEL_STACK_SIZE 	16384

#define MAX_DMA32_PHYS_ADDR 0xFFFFFFFFULL
#define MAX_DMA32_PAGE		(MAX_DMA32_PHYS_ADDR / PAGE_SIZE)
#define BITMAP_SIZE_BYTES	(TOTAL_DMA32_FRAMES / 8)

#define CANARY_MAGIC			0x485648494541500A
#define PMM_MIN_PHYS_ADDR 0x200000ULL
#define PMM_FIRST_FREE_FRAME (PMM_MIN_PHYS_ADDR / PAGE_SIZE)
typedef struct {
	void *ptr;
	uint64_t size;
	const char *file;
	int line;
}alloc_trace_t;

#define kmalloc(sz)				debug_kmalloc((sz), __FILE__, __LINE__)
#define kzalloc(sz)				debug_kzalloc((sz), __FILE__, __LINE__)
#define kcalloc(n, sz)			debug_kcalloc((n), (sz), __FILE__, __LINE__)

extern uint8_t kernel_stack[KERNEL_STACK_SIZE] __aligned(16);

typedef struct heap_header_s {
	uint64_t size;
	uint64_t is_free;
	struct heap_header_s *next;
} heap_header_t;


typedef struct kernel_memmap_s {
	uint64_t kernel_start;
	uint64_t kernel_size;
	uint64_t kheap_start;
	uint64_t kheap_size;
	uint64_t uheap_start;
	uint64_t uheap_size;
	uint64_t kstack_start;
	uint64_t kstack_size;
	uint64_t ustack_start;
	uint64_t ustack_size;
	uint64_t mem_start;
	uint64_t mem_size;	
} kernel_memmap_t;

typedef struct {
	uint64_t page_count;
	uint64_t requested_size;
} dma32_header_t;

typedef uint64_t pt_entry;

extern uint64_t			hhdm_offset;
extern pml4_table_t		*kernel_pml4;
extern uint64_t 		total_pages;
extern uint64_t 		used_pages;
extern uint64_t 		bitmap_size;
extern uint8_t			*map;
extern heap_header_t	*heap_start;
extern uint64_t			heap_current_limit;

/*bitmap.c*/
void bitmap_set(uint64_t page_index);
void bitmap_clear(uint64_t page_index);
int32_t bitmap_test(uint64_t page_index);

/*heap.c*/
void heap_init();
int heap_expand(uint64_t size_needed);

/*kmalloc.c*/
void *_kmalloc(uint64_t size);
void *debug_kmalloc(uint64_t sz, const char *file, int line);

void *kmalloc_dma32(uint64_t size);

void kfree(void *ptr);
void kfree_dma32(void *ptr);

void *_kcalloc(size_t n, size_t size);
void *debug_kcalloc(uint64_t n, uint64_t size, const char *file, int line);

void *krealloc(void *p, size_t new_n, size_t new_size, size_t old_total_size);

void *_kzalloc(size_t size);
void *debug_kzalloc(uint64_t size, const char *file, int line);

void mem_dump_alloc_trace(void);
void mem_verify_heap(void);
void mem_dump_pmm_stats(void);

/*pmm.c*/
void pmm_init(struct limine_memmap_response* memmap);
void *pmm_alloc();
void *pmm_alloc_n(uint64_t n);
void pmm_free(void* addr);
void *pmm_alloc_n_dma32(uint64_t n);
void *pmm_alloc_dma32(void);

/* memmap.c */
void init_mem(struct limine_memmap_response* memmap);
void map_page(pml4_table_t *pml4_virt, uint64_t vaddr, uint64_t paddr, uint64_t flags);
void map_page_wr(pml4_table_t *pml4_virt, uint64_t virt_addr_raw, uint64_t paddr);
pml4_table_t *create_new_pml4(void);


bool is_valid_user_address(const void *addr, size_t size);

kernel_memmap_t *init_kmemmap();
void print_kmemmap(kernel_memmap_t *km);

void *map_alloc_phys(size_t size);
void unmap_alloc_phys(void *ptr, size_t size);

static inline void tbl_invalidate(uint64_t vaddr) {
	__asm__ __volatile__("invlpg (%0)" :: "r"(vaddr) : "memory");
}



#endif