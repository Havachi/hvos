#ifndef HVOS_GDT_H
#define HVOS_GDT_H

#include <stdint.h>
#include "hvos/compiler_attributes.h"
#include "asm/segment.h"
#include "asm/desc_defs.h"
#include "kernel/tss.h"

typedef seg_descr_t gdt_desc_t;

typedef struct {
	uint16_t limit;
	uint64_t base;
} __packed gdt_ptr_t;

typedef struct {
	gdt_ptr_t ptr;
	__aligned(16) gdt_desc_t entries[GDT_ENTRIES];
	__aligned(16) tss_entry_t tss;
} gdt_t;

void set_gdt_gate(gdt_desc_t *entry, uint32_t base, uint32_t limit, uint32_t flags);
void init_gdt();
void init_gdt_local();
void tss_set_kernel_stack(uint64_t stack_ptr);
extern void reload_gdt(void);
extern void reload_tss(void);



#endif