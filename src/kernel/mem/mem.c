#include "mem/mem.h"
#include "kernel/boot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "limine.h"
#include "mem/lowmem.h"
#include "mem/paging.h"

#include <stddef.h>
#include <stdint.h>

uint64_t total_pages;
uint64_t used_pages;

uint64_t kernel_page_map;
extern uint8_t *map;
uint64_t bitmap_size;

uint64_t hhdm_offset;

static uint64_t top_ram;

extern void kernel_start;
extern void kernel_end;
uint64_t kernel_size;

heap_header_t *heap_start = NULL;
uint64_t heap_current_limit = 0xffffa00000000000;

pml4_table_t *kernel_pml4;

static uint64_t get_cr3() {
    uint64_t cr3;
    __asm__ __volatile("movq %%cr3, %0":"=r"(cr3));
    return cr3;
}

static void set_cr3(uint64_t cr3) {
    __asm__ __volatile("movq %0, %%cr3"::"r"(cr3));
}

static uint64_t get_highest_phys_addr(struct limine_memmap_response *memmap) {
    uint64_t max_addr = 0;
    for (uint32_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_BAD_MEMORY) continue;
		uint64_t top = e->base + e->length;
		if (top > max_addr) max_addr = top;
    }
    return max_addr;
}

void init_bitmap(struct limine_memmap_response *memmap) {
    uint64_t max_addr = get_highest_phys_addr(memmap);
    total_pages = max_addr / PAGE_SIZE;
    bitmap_size = PAGE_ALIGN_UP((total_pages + 7) / 8);

    uint64_t bitmap_phys = 0;

    // 1. Locate usable region >= 2 MiB without mutating Limine's struct
    for (uint32_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start = PAGE_ALIGN_UP(e->base);
            if (start < PMM_MIN_PHYS_ADDR) start = PMM_MIN_PHYS_ADDR;

            if ((e->base + e->length) >= (start + bitmap_size)) {
                bitmap_phys = start;
                map = (uint8_t *)(PHYS_TO_VIRT(bitmap_phys));
                break;
            }
        }
    }

    if (!bitmap_phys) {
        printf("[PMM Panic] Failed to allocate bitmap above 2MB\n");
        for (;;) __asm__ volatile("cli; hlt");
    }

    // 2. Default all frames to USED (0xFF)
    memset(map, 0xFF, bitmap_size);

    // 3. Mark usable RAM ranges as FREE (0)
    for (uint32_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start = PAGE_ALIGN_UP(e->base);
            uint64_t end   = PAGE_ALIGN_DOWN(e->base + e->length);

            for (uint64_t phys = start; phys < end; phys += PAGE_SIZE) {
                if (phys >= PMM_MIN_PHYS_ADDR) {
                    bitmap_clear(phys / PAGE_SIZE);
                }
            }
        }
    }

    // 4. Reserve the bitmap's own physical frames
    uint64_t bitmap_start_frame = bitmap_phys / PAGE_SIZE;
    uint64_t bitmap_frame_count = bitmap_size / PAGE_SIZE;
    for (uint64_t f = 0; f < bitmap_frame_count; f++) {
        bitmap_set(bitmap_start_frame + f);
    }
}

uintptr_t alloc_page_table(void) {
    uintptr_t phys_addr = (uintptr_t)pmm_alloc();
    if (!phys_addr) {
        printf("[VMM] Failed to allocate physical memory for page table\n");
        for (;;) __asm__ __volatile__("cli; hlt");
    }

    uint64_t *virt_addr = (uint64_t *)PHYS_TO_VIRT(phys_addr);
    memset(virt_addr, 0, PAGE_SIZE);
    return phys_addr;
}

///Map a Virtual address to a Physical Address
void map_page(pml4_table_t *pml4_virt, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    virtual_address_t virt = { ._raw = vaddr };
    
    uint64_t indices[4] = {
        virt.pml4_index,
        virt.pdpt_index,
        virt.pd_index,
        virt.pt_index,
    };

    uint64_t *table = (uint64_t *)pml4_virt;

    for (int level = 0; level < 3; level++) {
        uint64_t entry = table[indices[level]];

        // If entry missing OR is a Huge Page, allocate a clean child page table
        if (!(entry & PTE_PRESENT) || (entry & PTE_PSE)) {
            uintptr_t new_table_phys = alloc_page_table();
            uint64_t parent_flags = PTE_PRESENT | PTE_WRITABLE;
            if (flags & PTE_USER) parent_flags |= PTE_USER;
            
            table[indices[level]] = new_table_phys | parent_flags;
            table = (uint64_t *)PHYS_TO_VIRT(new_table_phys);
        } else {
            if (flags & PTE_USER) {
                table[indices[level]] |= PTE_USER;
            }
            table = (uint64_t *)PHYS_TO_VIRT(entry & PTE_ADDR_MASK);
        }
    }

    bool updating_existing = (table[indices[3]] & PTE_PRESENT) != 0;
    table[indices[3]] = (paddr & PTE_ADDR_MASK) | flags;

    if (updating_existing) {
        tbl_invalidate(vaddr);
    }
}
///Map the whole usable ram
static void map_ram(struct limine_memmap_response *memmap) {
    for (uint32_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type == LIMINE_MEMMAP_BAD_MEMORY) continue;

        uint64_t base = PAGE_ALIGN_DOWN(e->base);
        uint64_t top = PAGE_ALIGN_UP(e->base + e->length);
        for (uint64_t phys = base; phys < top; phys += PAGE_SIZE) {
            map_page(kernel_pml4, PHYS_TO_VIRT(phys), phys, PTE_PRESENT | PTE_WRITABLE);
        }
    }
}

static void map_kernel(uint64_t kpaddr, uint64_t kvaddr, size_t ksize) {
	for (uint64_t offset = 0; offset < ksize; offset += PAGE_SIZE) {
		uint64_t paddr = kpaddr + offset;
		map_page(kernel_pml4, (((uint64_t)kvaddr) + offset), paddr, PTE_PRESENT | PTE_WRITABLE);
	}
}


pml4_table_t *create_new_pml4(void) {
    uint64_t pml4_phys = (uint64_t)pmm_alloc();
	if (!pml4_phys) return NULL;

	pml4_table_t *pml4_virt = (pml4_table_t *)PHYS_TO_VIRT(pml4_phys);
    memset(pml4_virt, 0, PAGE_SIZE);

	for (int i = 256; i < 512; i++) {
		pml4_virt->entries[i] = kernel_pml4->entries[i];
	}
    return pml4_virt;
}


void init_mem(struct limine_memmap_response *memmap) {
	//Globally set hhdm offset for further uses
	hhdm_offset = hhdm_request.response->offset;
	top_ram = get_highest_phys_addr(memmap);
	kernel_size = PAGE_ALIGN_UP(((uint64_t)&kernel_end - (uint64_t)&kernel_start));

	init_bitmap(memmap);

	uint64_t new_pml4_phys = (uint64_t)pmm_alloc();
	kernel_pml4 = (pml4_table_t *)PHYS_TO_VIRT(new_pml4_phys);
	memset(kernel_pml4, 0, PAGE_SIZE);

	map_ram(memmap);

	uint64_t kernel_paddr = kernel_address_request.response->physical_base;
	uint64_t kernel_vaddr = kernel_address_request.response->virtual_base;

	map_kernel(kernel_paddr, kernel_vaddr, kernel_size);

	set_cr3(new_pml4_phys);

	heap_init();
}

bool is_valid_user_address(const void *addr, size_t size) {
	uintptr_t start = (uintptr_t)addr;
	uintptr_t end = start + size;
	if (end < start) {
		return false;
	}

	if (end > USER_SPACE_END) {
		return false;
	}

	return true;
}

kernel_memmap_t *init_kmemmap() {
	kernel_memmap_t *memmap = kzalloc(sizeof(kernel_memmap_t));

	memmap->kernel_start = (uint64_t)&kernel_start;
	memmap->kernel_size = kernel_size;

	memmap->kstack_start = (uint64_t)&kernel_stack;
	memmap->kstack_size = (uint64_t)KERNEL_STACK_SIZE;

	memmap->kheap_start = (uint64_t)&heap_start;
	memmap->kheap_size = (uint64_t)(heap_current_limit - ((uint64_t)&heap_start));

	return memmap;
}

void print_kmemmap(kernel_memmap_t *km) {
    printf("\nKERNEL MEMMAP\n");
	printf("kernel:\t\t%016p\t%016p\n", km->kernel_start, km->kernel_start + km->kernel_size);
	printf("stack:\t\t%016p\t%016p\n", km->kstack_start, km->kstack_start - km->kstack_size);
	printf("heap:\t\t%016p\t%016p\n", km->kheap_start, km->kheap_start + km->kheap_size);
	printf("user stack:\t%016p\t%016p\n", km->ustack_start, km->ustack_start - km->ustack_size);
}
