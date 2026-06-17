#ifndef HVOS_ASM_DESC_H
#define HVOS_ASM_DESC_H

#include "asm/segment.h"
#include "asm/desc_defs.h"
#include "kernel/idt.h"
#include "hvos/compiler_attributes.h"
#include "mem/mem.h"

#define __force
typedef struct gdt_page {
	seg_descr_t gdt[GDT_ENTRIES];
}__aligned(PAGE_SIZE) __packed gdt_page_t;

static inline void load_gdt(const struct desc_ptr *gdtr) {
	__asm__ __volatile("lgdt %0"::"m"(*gdtr));
}

static __always_inline void load_idt(const idtr_t *idtr) {
	__asm__ __volatile("lidt %0"::"m" (*idtr));
}


#endif