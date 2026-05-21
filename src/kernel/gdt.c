#include "kernel/gdt.h"
#include "kernel/tss.h"
#include "asm/segment.h"
#include "asm/desc.h"
#include "asm/asm.h"
#include "klibc/string.h"
#include "asm/asm.h"

gdtr_t gdt = {0};
gdt_ptr_t gdt_ptr = {0};
tss_t global_tss = {0};

uint64_t syscall_kernel_stack = 0;

gdt_page_t gdt_page = {
	.gdt = {
		[0] = 0,
		[GDT_ENTRY_KERNEL_CS] = GDT_ENTRY_INIT(DESC_CODE64, 0, 0xFFFFF),
		[GDT_ENTRY_KERNEL_DS] = GDT_ENTRY_INIT(DESC_DATA64, 0, 0xFFFFF),
		[GDT_ENTRY_DEFAULT_USER_DS] = GDT_ENTRY_INIT(DESC_DATA64 | DESC_USER, 0, 0xFFFFF),
		[GDT_ENTRY_DEFAULT_USER_CS] = GDT_ENTRY_INIT(DESC_CODE64 | DESC_USER, 0, 0xFFFFF),
	}
};

static void set_gdt_tss_gate (uint64_t base) {
	uint64_t limit = sizeof(tss_t) - 1;
	uint64_t low = 0;

	low |= (limit & 0xFFFF);
	low |= (base & 0xFFFF) << 16;
	low |= ((base >> 16) & 0xFF) << 32;
	low |= (uint64_t)0x89 << 40;
	low |= ((limit >> 16) & 0x0F) << 48;
	low |= ((base >> 24) & 0xFF) << 56;

	uint64_t high = (base >> 32) & 0xFFFFFFFF;

	gdt.tss_low = low;
	gdt.tss_high = high;
}

void init_gdt(uint64_t _hddm_offset, void *pmm_allocated_page) {

	//[0x0] NULL SEGM

	set_gdt_gate(0, 0, 0, 0, 0);
	set_gdt_gate(GDT_ENTRY_KERNEL_CS, 0x0, 0xFFFFF, 0x9A, 0xAF);
	set_gdt_gate(GDT_ENTRY_KERNEL_DS, 0x0, 0xFFFFF, 0x92, 0xCF);
	set_gdt_gate(GDT_ENTRY_DEFAULT_USER_DS, 0x0, 0xFFFFF, 0xF2, 0xCF);
	set_gdt_gate(GDT_ENTRY_DEFAULT_USER_CS, 0x0, 0xFFFFF, 0xFA, 0xAF);
	kmemset(&global_tss, 0, sizeof(tss_t));

	uint64_t kernel_stack_top = (uint64_t)pmm_allocated_page + hhdm_offset + 4096;
	global_tss.rsp0 = kernel_stack_top;

	syscall_kernel_stack = kernel_stack_top;

	set_gdt_tss_gate((uint64_t)&global_tss);

	gdt_ptr.limit = (sizeof(gdt) - 1);
	gdt_ptr.base = (uint64_t) &gdt;

	reload_gdt();
	reload_tss();
}

void newinit_gdt(uint64_t _hddm_offset, void *pmm_allocated_page) {
	struct gdt_page *gp = rip_rel_ptr((void *)(__force uint64_t)&gdt_page);
	void *handler = NULL;

	struct desc_ptr s = {
		.address = (uint64_t)gp->gdt,
		.size = GDT_SIZE - 1,
	};

	load_gdt(&s);

	asm volatile("movl %%eax, %%ds\n"
				 "movl %%eax, %%ss\n"
				 "movl %%eax, %%es\n" : : "a"(__KERNEL_DS) : "memory");
}


void set_gdt_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
	gdt.entries[num].base_low = (base & 0xFFFF);
	gdt.entries[num].base_middle = (base >> 16) & 0xFF;
	gdt.entries[num].base_high = (base >> 24) & 0xFF; 
	gdt.entries[num].limit_low = (limit & 0xFFFF);
	gdt.entries[num].granularity = (limit >> 16) & 0x0F;
	gdt.entries[num].granularity |= granularity & 0xF0;	
	gdt.entries[num].access = access;
}

void tss_set_kernel_stack(uint64_t stack_ptr) {
	global_tss.rsp0 = stack_ptr;
	syscall_kernel_stack = stack_ptr;
}