#include "kernel/mem.h"
#include <string.h>

// Globals linked via extern from kernel/mem.h
uint8_t *bitmap;
uint64_t bitmap_size;
uint64_t total_pages;

heap_header_t *heap_start = NULL;
uint64_t heap_current_limit = 0xffffa00000000000;

uint64_t hhdm_offset;
volatile pt_entry *current_pml4;
uint64_t kernel_pml4_phys = 0;

// Imported from your centralized boot_requests.c
extern volatile struct limine_kernel_address_request kernel_address_request;



static void *get_virt(uint64_t phys) {
	return (void *)(phys + hhdm_offset);
}

void display_memmap_debug(uint64_t nb_entries, struct limine_memmap_entry **entries) {
    kprintf("mem map (%d)\n", nb_entries);
    kprintf("ID\t%-16s\t%-9s\tTYPE\n", "BASE", "SIZE");
    for (int i = 0; i < nb_entries; i++) {
        if (entries[i]->length == 0) continue;
        kprintf("[%d]\t%016p\t%09x\t", i, entries[i]->base, entries[i]->length);
        switch (entries[i]->type) {
            case LIMINE_MEMMAP_USABLE:                 kprintf("USABLE"); break;
            case LIMINE_MEMMAP_RESERVED:               kprintf("RESERVED"); break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:       kprintf("ACPI_RECLAIMABLE"); break;
            case LIMINE_MEMMAP_ACPI_NVS:               kprintf("ACPI_NVS"); break;
            case LIMINE_MEMMAP_BAD_MEMORY:             kprintf("BAD_MEMORY"); break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: kprintf("BOOTLOADER_RECLAIMABLE"); break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:     kprintf("EXECUTABLE_AND_MODULES"); break;
            case LIMINE_MEMMAP_FRAMEBUFFER:            kprintf("FRAMEBUFFER"); break;
            default:                                   kprintf("UNKNOWN"); break;
        }
        kprintf("\n");
    }
}

void vmm_map(pt_entry* pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PTE_PRESENT)) {
        uint64_t new_table = (uint64_t)pmm_alloc();
        memset(get_virt(new_table), 0, PAGE_SIZE);
        pml4[pml4_idx] = new_table | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    pt_entry *pdpt = get_virt(pml4[pml4_idx] & PTE_ADDR_MASK);

    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) {
        uint64_t new_table = (uint64_t)pmm_alloc();
        memset(get_virt(new_table), 0, PAGE_SIZE);
        pdpt[pdpt_idx] = new_table | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    pt_entry* pd = get_virt(pdpt[pdpt_idx] & PTE_ADDR_MASK);

    if (!(pd[pd_idx] & PTE_PRESENT)) {
        uint64_t new_table = (uint64_t)pmm_alloc();
        memset(get_virt(new_table), 0, PAGE_SIZE);
        pd[pd_idx] = new_table | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    pt_entry* pt = get_virt(pd[pd_idx] & PTE_ADDR_MASK);

    pt[pt_idx] = phys | flags | PTE_PRESENT;
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void init_mem(struct limine_memmap_response* memmap, uint64_t _hhdm_offset) {
    hhdm_offset = _hhdm_offset;
    pmm_init(memmap);

    uint64_t old_pml4_phys;
    asm volatile("mov %%cr3, %0" : "=r"(old_pml4_phys));
    pt_entry* old_pml4 = (pt_entry*)(old_pml4_phys + hhdm_offset);
    kernel_pml4_phys = (uint64_t)pmm_alloc();
    pt_entry* new_pml4 = (pt_entry*)(kernel_pml4_phys + hhdm_offset);
    memset(new_pml4, 0, 4096);

    for (int i = 256; i < 512; i++) new_pml4[i] = old_pml4[i];
    for (int i = 0; i < 4; i++)    new_pml4[i] = old_pml4[i];

    //LAPIC
    vmm_map(new_pml4, 0xFEE00000 + hhdm_offset, 0xFEE00000, PTE_PRESENT | PTE_WRITABLE | PTE_NX);
    //IO APIC
    vmm_map(new_pml4, 0xFEC00000 + hhdm_offset, 0xFEC00000, PTE_PRESENT | PTE_WRITABLE | PTE_NX);
    current_pml4 = new_pml4;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE &&
            entry->type != LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE &&
            entry->type != LIMINE_MEMMAP_KERNEL_AND_MODULES &&
            entry->type != LIMINE_MEMMAP_FRAMEBUFFER) {
            continue;
        }

        if (entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            uint64_t base_aligned = entry->base & ~(PAGE_SIZE - 1);
            uint64_t length_aligned = (entry->length + (entry->base - base_aligned) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            for (uint64_t j = 0; j < length_aligned; j += PAGE_SIZE) {
                vmm_map(
					new_pml4,
					base_aligned + j + hhdm_offset,
					base_aligned + j,
					PTE_PRESENT | PTE_WRITABLE | PTE_NX | PTE_PCD | PTE_PWT
				);
            }
            continue;
        }

        uint64_t rounded_len = (entry->length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        for (uint64_t j = 0; j < rounded_len; j += PAGE_SIZE) {
            uint64_t phys = entry->base + j;
            uint64_t virt = phys + hhdm_offset;
            vmm_map(new_pml4, virt, phys, PTE_PRESENT | PTE_WRITABLE);
        } 
    }

    struct limine_kernel_address_response *kaddr = kernel_address_request.response; 
    uint64_t kphys = kaddr->physical_base;
    uint64_t kvirt = kaddr->virtual_base;
    uint64_t ksize = 0;
    
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES && entry->base == kphys) {
            ksize = entry->length;
            break;
        }
    }
    if (ksize == 0) ksize = 0x200000; 
    for (uint64_t i = 0; i < ksize; i += PAGE_SIZE) {
        vmm_map(new_pml4, kvirt + i, kphys + i, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    }

    asm volatile("mov %0, %%cr3" :: "r"(kernel_pml4_phys) : "memory");
    heap_init(new_pml4);
}

uint64_t vmm_create_address_space(void) {
    uint64_t new_pml4_phys = (uint64_t) pmm_alloc();
    pt_entry *new_pml4 = (pt_entry *)(new_pml4_phys + hhdm_offset);

    memset(new_pml4, 0 , PAGE_SIZE);
    extern uint64_t kernel_pml4_phys;
    pt_entry *kernel_pml4 = (pt_entry *)(kernel_pml4_phys + hhdm_offset);
    for (int i = 256; i < 512; i++) {
        new_pml4[i] = kernel_pml4[i];
    }

    return new_pml4_phys;
}