#include "mem/mem.h"
#include "kernel/boot.h"
#include <stdio.h>
#include <string.h>
#include "limine.h"
#include "mem/paging.h"

#include <stddef.h>
#include <stdint.h>

uint64_t total_pages;
uint64_t used_pages;

uint64_t kernel_page_map;
extern uint8_t *bitmap;
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
    asm volatile("movq %%cr3, %0":"=r"(cr3));
    return cr3;
}

static void set_cr3(uint64_t cr3) {
    asm volatile("movq %0, %%cr3"::"r"(cr3));
}

static virtual_address_t *new_virt(uint64_t virt) {
	virtual_address_t *virtaddr = (virtual_address_t *)virt;
	return virtaddr;
}

void memory_frame_set(uint64_t frame, uint64_t length) {
	while (length > 0) {
		if (frame % 8 == 0 && length >= 8) {
			bitmap[frame / 8] = 0xFF;
			frame += 8;
			length -= 8;
		} else {
			bitmap[frame / 8] |= (1 << (7 - (frame % 8)));
			frame++;
			length--;
		}
	}
}

static uint64_t get_highest_phys_addr(struct limine_memmap_response *memmap) {
	uint64_t max_addr = 0;
	for (uint32_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *e = memmap->entries[i];
		if (e->type == LIMINE_MEMMAP_RESERVED) continue;
		if (e->base + e->length > max_addr)
			max_addr = e->base + e->length;
	}
	return max_addr;
}

void init_bitmap(struct limine_memmap_response *memmap) {
	uint64_t max_addr = get_highest_phys_addr(memmap);
	uint64_t pages = 0;
	bitmap_size = (max_addr / PAGE_SIZE) / 8;

	for (uint32_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *e = memmap->entries[i];
		if (e->type == LIMINE_MEMMAP_USABLE && e->length >= bitmap_size){
			bitmap = (uint8_t *)(PHYS_TO_VIRT(e->base));
			e->base += bitmap_size;
			e->length -= bitmap_size;
			break;
		}
	}
	if (bitmap == NULL){
		printf("FAILED TO ALLOCATE BITMAP");
		for (;;){
			asm volatile("hlt");
		}
	}
	memset(bitmap, 0xFF, bitmap_size);
	for (uint32_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *e = memmap->entries[i];
		if (e->type == LIMINE_MEMMAP_USABLE){
			for (uint64_t addr = 0; addr < e->length; addr += PAGE_SIZE){
				bitmap_clear((e->base + addr) / PAGE_SIZE);
				pages++;
			}
		}
	}
	total_pages = pages;
}


uint64_t *alloc_page_table() {
	uint64_t phys_addr = (uint64_t)pmm_alloc();
	if (!phys_addr) return NULL;

	uint64_t *virt_addr = (uint64_t *)PHYS_TO_VIRT(phys_addr);
	memset(virt_addr, 0, PAGE_SIZE);
	return (uint64_t *)phys_addr;
}

///Map a Virtual address to a Physical Address
void map_page(pml4_table_t *pml4_virt, uint64_t virt_addr_raw, uint64_t paddr, uint64_t flags) {
		
		virtual_address_t vaddr = {._raw = virt_addr_raw};

		page_table_entry_t *pml4_entry = &pml4_virt->entries[vaddr.pml4_index]; 
		if (!pml4_entry->present) {
			uint64_t *new_table = alloc_page_table();
			if (new_table == NULL) {
				printf("KERNEL PANIC: Out of physical memory\n");
				for(;;) asm volatile("hlt");
			}
			pml4_entry->present = 1;
			pml4_entry->rw = 1;
			pml4_entry->us = 0;
			pml4_entry->address = (uint64_t)new_table >> 12;
			pml4_entry->_raw = pml4_entry->_raw | flags;
		}
		pdpt_table_t *pdpt = (pdpt_table_t *)(uint64_t)PHYS_TO_VIRT(pml4_entry->address << 12);
		page_table_entry_t *pdpt_entry = &pdpt->entries[vaddr.pdpt_index];

		if (!pdpt_entry->present) {
			uint64_t *new_table = alloc_page_table();
			if (new_table == NULL) {
				printf("KERNEL PANIC: Out of physical memory\n");
				for(;;) asm volatile("hlt");
			}
			pdpt_entry->present = 1;
			pdpt_entry->rw = 1;
			pdpt_entry->us = 0;
			pdpt_entry->address = (uint64_t)new_table >> 12;
			pdpt_entry->_raw = pdpt_entry->_raw | flags;
		}
		pd_table_t *pd = (pd_table_t *)(uint64_t) PHYS_TO_VIRT(pdpt_entry->address << 12);
		page_table_entry_t *pd_entry = &pd->entries[vaddr.pd_index];

		if (!pd_entry->present) {
			uint64_t *new_table = alloc_page_table();
			if (new_table == NULL) {
				printf("KERNEL PANIC: Out of physical memory\n");
				for(;;) asm volatile("hlt");
			}
			pd_entry->present = 1;
			pd_entry->rw = 1;
			pd_entry->us = 0;
			pd_entry->address = (uint64_t)new_table >> 12;
			pd_entry->_raw = pd_entry->_raw | flags;
		}

		pt_table_t *pt = (pt_table_t *)(uint64_t) PHYS_TO_VIRT(pd_entry->address << 12);
		page_table_entry_t *pt_entry = &pt->entries[vaddr.pt_index];
		
		pt_entry->present = 1;
		pt_entry->rw = 1;
		pt_entry->us = 0;
		pt_entry->address = paddr >> 12;
		pt_entry->_raw = pt_entry->_raw | flags;

}
///Map the whole usable ram
static void map_ram(struct limine_memmap_response *memmap) {
	for (uint32_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *e = memmap->entries[i];
		if (e->type == LIMINE_MEMMAP_RESERVED) continue;
		uint64_t base = e->base;
		uint64_t length = e->length;
		for (uint64_t phys = base; phys < (base + length); phys += PAGE_SIZE) {
			map_page(kernel_pml4, PHYS_TO_VIRT(phys), phys, PTE_WRITABLE);
		}
	}
}

static void map_kernel(uint64_t kpaddr, virtual_address_t *kvaddr, size_t ksize) {
	for (uint64_t offset = 0; offset < ksize; offset += PAGE_SIZE) {
		uint64_t paddr = kpaddr + offset;
		map_page(kernel_pml4, (((uint64_t)kvaddr) + offset), paddr, PTE_WRITABLE);
	}
}

pml4_table_t *create_new_pml4(void) {

    uint64_t pml4_phys = (uint64_t)pmm_alloc();
    pml4_table_t *pml4_virt = (pml4_table_t *)PHYS_TO_VIRT(pml4_phys);

    memset(pml4_virt, 0, PAGE_SIZE);
	memcpy(pml4_virt, kernel_pml4, PAGE_SIZE);
    return pml4_virt;
}


void init_mem(struct limine_memmap_response *memmap) {
	//Globally set hhdm offset for further uses
	hhdm_offset = hhdm_request.response->offset;
	top_ram = get_highest_phys_addr(memmap);
	kernel_size = ((uint64_t)&kernel_end - (uint64_t)&kernel_start);

	init_bitmap(memmap);

	uint64_t old_cr3 = get_cr3() & ~(uint64_t)0xFFF;
	pml4_table_t *old_pml4 = (pml4_table_t *)PHYS_TO_VIRT(old_cr3);

	uint64_t new_pml4_phys = (uint64_t)pmm_alloc();
	kernel_pml4 = (pml4_table_t *)PHYS_TO_VIRT(new_pml4_phys);

	memcpy(kernel_pml4, old_pml4, PAGE_SIZE);

	map_ram(memmap);

	uint64_t kernel_paddr = kernel_address_request.response->physical_base;
	virtual_address_t *kernel_vaddr = (virtual_address_t*)kernel_address_request.response->virtual_base;
	map_kernel(kernel_paddr, kernel_vaddr, kernel_size);

	uint64_t rsp;
	asm volatile("mov %%rsp, %0" : "=r"(rsp));
	if (!(rsp >= hhdm_offset && rsp < (hhdm_offset + top_ram)) && !(rsp >= (uint64_t)kernel_vaddr && rsp < ((uint64_t) kernel_vaddr + kernel_size))){
		//should map stack
		printf("STACK NOT MAPPED !");
		for(;;) {
			asm volatile("hlt");
		}
	}
	set_cr3((uint64_t)new_pml4_phys);
	heap_init();
}