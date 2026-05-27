#include "kernel/gdt.h"
#include "kernel/local_apic.h"
#include "asm/segment.h"
#include "asm/desc.h"
#include "asm/asm.h"
#include "kernel/smp.h"
#include "klibc/string.h"
#include <stdint.h>
#include "mem/paging.h"

static gdt_t **gdt_list;

uint64_t syscall_kernel_stack = 0;


static void set_gdt_tss_gate (gdt_desc_t* entry, uint64_t base, uint64_t limit) {

	gdt_desc_t *tss_1 = entry;
	gdt_desc_t *tss_2 = entry + 1;
	kmemset(tss_1, 0, sizeof(gdt_desc_t));
	kmemset(tss_1, 0, sizeof(gdt_desc_t));

	tss_1->limit0 = limit & 0xFFFF;
	tss_1->base0 = base & 0xFFFF;
	tss_1->base1 = (base >> 16) & 0xFF;
	tss_1->type = 0x9;
	tss_1->s = 0; 
	tss_1->dpl = 0; 
	tss_1->p = 1; 
	tss_1->limit1 = (limit >> 16) & 0x0F;
	tss_1->avl = 0;
	tss_1->l = 0;
	tss_1->d = 0;
	tss_1->g = 0;
	tss_1->base2 = (base >> 24) & 0xFF;

	uint32_t base_upper = (base >> 32) & 0xFFFFFFFF;
	tss_2->limit0 = base_upper & 0xFFFF;
	tss_2->base0 = (base_upper >> 16) & 0xFFFF;

}




void set_gdt_gate(gdt_desc_t *entry, uint32_t base, uint32_t limit, uint32_t flags) {
	entry->base0 	= (base & 0xFFFF);
	entry->base1 	= (base >> 16) & 0x00FF;
	entry->base2 	= (base >> 24) & 0x00FF; 
	entry->limit0 	= (limit & 0xFFFF);
	entry->limit1 	= ((limit >> 16) & 0xFFFF);
	entry->type 	= flags & 0x000F;
	entry->type		= ((flags)	>>  0) & 0x000F;
	entry->s		= ((flags)	>>  4) & 0x0001;
	entry->dpl		= ((flags)	>>  5) & 0x0003;
	entry->p		= ((flags)	>>  7) & 0x0001;
	entry->avl		= ((flags)	>> 12) & 0x0001;
	entry->l		= ((flags)	>> 13) & 0x0001;
	entry->d		= ((flags)	>> 14) & 0x0001;
	entry->g		= ((flags)	>> 15) & 0x0001;
}

void init_gdt(){
	uint32_t cores = g_acpi_cpu_count;
	gdt_list = (gdt_t **) kmalloc(sizeof(gdt_t) * cores);
	for (uint32_t i = 0; i < cores; i++) {
		size_t size = sizeof(gdt_t) * 2;
		void *gdt_mem = kmalloc(size);
		kmemset(gdt_mem, 0, size);
		gdt_list[i] = (gdt_t *) ALIGN_BOUND((uint64_t)gdt_mem, 0x10);
	}
}

void tss_set_kernel_stack(uint64_t stack_ptr) {
	gdt_t *local_gdt = gdt_list[local_apic_get_id()];
	local_gdt->tss.rsp0 = stack_ptr;
	syscall_kernel_stack = stack_ptr;
}

void init_gdt_local(){

	gdt_t *local_gdt = gdt_list[local_apic_get_id()];
	local_gdt->ptr.limit = GDT_SIZE - 1;
	local_gdt->ptr.base = (uint64_t)&local_gdt->entries;

	set_gdt_gate(&local_gdt->entries[0],0,0,0);
	set_gdt_gate(&local_gdt->entries[GDT_ENTRY_KERNEL_CS],0,0xFFFFFFFF,DESC_CODE64);
	set_gdt_gate(&local_gdt->entries[GDT_ENTRY_KERNEL_DS],0,0xFFFFFFFF,DESC_DATA64);
	set_gdt_gate(&local_gdt->entries[GDT_ENTRY_DEFAULT_USER_DS],0,0xFFFFFFFF,DESC_DATA64 | DESC_USER);
	set_gdt_gate(&local_gdt->entries[GDT_ENTRY_DEFAULT_USER_CS],0,0xFFFFFFFF,DESC_CODE64 | DESC_USER);


	set_gdt_tss_gate(&local_gdt->entries[GDT_ENTRY_TSS], (uint64_t) &local_gdt->tss, sizeof(tss_entry_t));
	kmemset(&local_gdt->tss, 0, sizeof(tss_entry_t));
	local_gdt->tss.rsp0 = 0;

	asm volatile("lgdt %0" : : "m" (local_gdt->ptr));
	asm volatile(
		"movw %0, %%ax;"
		"movw %%ax, %%ds;"
		"movw %%ax, %%es;"
		"movw %%ax, %%fs;"
		"movw %%ax, %%gs;"
		"movw %%ax, %%ss;"
		"pushq %1;"
		"pushq $reloadcs;"
		"lretq;"
		"reloadcs:"
		 : : "i"(__KERNEL_DS), "i"(__KERNEL_CS): "ax", "memory");
	asm volatile(
		"mov %0, %%ax;"
		"ltr %%ax;"
		::"i"(__TSS_SEG) : "memory");
}