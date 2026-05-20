#ifndef HVOS_GDT_H
#define HVOS_GDT_H

#include <stdint.h>


typedef struct gdt_entry_s {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;


typedef struct tss_entry_s {
	uint16_t length;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t flags1;
	uint8_t flags2;
	uint8_t base_hi;
	uint32_t base_upper32;
	uint32_t reserved;
} __attribute__((packed)) tss_entry_t;

typedef struct gdt_ptr_s {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

typedef struct gdtr_s
{
	gdt_entry_t entries[5];
	uint64_t tss_low;
	uint64_t tss_high;
} __attribute__((packed)) gdtr_t;



void set_gdt_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
void init_gdt(uint64_t hddm_offset, void *pmm_allocated_page);
void tss_set_kernel_stack(uint64_t stack_ptr);
extern void reload_gdt(void);
extern void reload_tss(void);

#endif