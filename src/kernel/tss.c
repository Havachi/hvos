#include "kernel/tss.h"
#include "mem/mem.h"
#include "klibc/string.h"

static tss_t global_tss;

extern void load_tss(uint16_t selector);

void tss_init(void) {
	kmemset(&global_tss, 0, sizeof(tss_t));
	uint64_t safe_stack_phys = (uint64_t)pmm_alloc();
	global_tss.rsp0 = safe_stack_phys + hhdm_offset + PAGE_SIZE;
}

void tss_set_stack(uint64_t kernel_stack) {
	global_tss.rsp0 = kernel_stack;
}