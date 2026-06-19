#ifndef HVOS_MEM_H
#define HVOS_MEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "limine.h"
#include "mem/paging.h"


#define PAGE_SIZE		4096
#define PTE_PRESENT		(1ULL << 0)
#define PTE_WRITABLE	(1ULL << 1)
#define PTE_USER		(1ULL << 2)
#define PTE_PWT			(1ULL << 3)
#define PTE_PCD			(1ULL << 4)
#define PTE_NX			(1ULL << 63)
#define PTE_ADDR_MASK	0x000ffffffffff000
#define USR_STACK_BASE	0x00007FFFFFFFF000
#define KERNEL_HEAP_START 0xffffa00000000000
#define USR_HEAP_BASE	0x10000000
#define PHYS_TO_VIRT(phys) ((uint64_t)(phys) + hhdm_offset)
#define VIRT_TO_PHYS(virt) (((uint64_t)(virt) != 0) ? ((uint64_t)(virt) - hhdm_offset) : (uint64_t)(virt))


typedef struct heap_header_s {
	uint64_t size;
	bool is_free;
	struct heap_header_s *next;
} heap_header_t;


typedef uint64_t pt_entry;

extern uint64_t			hhdm_offset;
extern pml4_table_t		*kernel_pml4;
extern uint64_t 		total_pages;
extern uint64_t 		used_pages;
extern uint64_t 		bitmap_size;
extern uint8_t			*bitmap;
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
void *kmalloc(uint64_t size);
void kfree(void *ptr);
void *kcalloc(size_t n, size_t size);
void *krealloc(void *p, size_t new_n, size_t new_size, size_t old_total_size);
void *kzalloc(size_t size);
/*pmm.c*/
void pmm_init(struct limine_memmap_response* memmap);
void *pmm_alloc();
void *pmm_alloc_n(uint64_t n);
void pmm_free(void* addr);


/* memmap.c */
void init_mem(struct limine_memmap_response* memmap);
void map_page(pml4_table_t *pml4_virt, uint64_t vaddr, uint64_t paddr, uint64_t flags);
pml4_table_t *create_new_pml4(void);


bool is_valid_user_address(const void *addr, size_t size);
#endif