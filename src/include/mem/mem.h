#ifndef HVOS_MEM_H
#define HVOS_MEM_H

#include <stdint.h>
#include <stdbool.h>
#include "limine.h"


#define PAGE_SIZE		4096
#define PTE_PRESENT		(1ULL << 0)
#define PTE_WRITABLE	(1ULL << 1)
#define PTE_USER		(1ULL << 2)
#define PTE_PWT			(1ULL << 3)
#define PTE_PCD			(1ULL << 4)
#define PTE_NX			(1ULL << 63)
#define PTE_ADDR_MASK	0x000ffffffffff000
#define TO_VIRT(phys)	((void *)((uint64_t)(phys) + hhdm_offset))

typedef struct heap_header_s {
	uint64_t size;
	bool is_free;
	struct heap_header_s *next;
} heap_header_t;


typedef uint64_t pt_entry;

extern uint64_t hhdm_offset;
extern volatile pt_entry *current_pml4;
extern uint64_t kernel_pml4_phys;
extern uint64_t total_pages;
extern uint64_t bitmap_size;
extern uint8_t *bitmap;
extern heap_header_t *heap_start;
extern uint64_t heap_current_limit;

/*bitmap.c*/
void bitmap_set(uint64_t page_index);
void bitmap_clear(uint64_t page_index);
int32_t bitmap_test(uint64_t page_index);

/*heap.c*/

void heap_init(volatile pt_entry *pml4);
void heap_expand(uint64_t size_needed);

/*kmalloc.c*/
void *kmalloc(uint64_t size);
void kfree(void *ptr);

/*pmm.c*/
void pmm_init(struct limine_memmap_response* memmap);
void *pmm_alloc();
void pmm_free(void* addr);


void display_memmap_debug (uint64_t nb_entries, struct limine_memmap_entry **entries);
void init_mem(struct limine_memmap_response* memmap, uint64_t _hhdm_offset);
void vmm_map(volatile pt_entry* pml4, uint64_t virt, uint64_t phys, uint64_t flags);
uint64_t vmm_create_address_space(void);

#endif