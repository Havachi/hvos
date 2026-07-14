#include "drivers/ata.h"
#include "drivers/pci.h"
#include "kernel/boot.h"
#include "kernel/elf.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/intr.h"
#include "kernel/isr.h"
#include "kernel/scheduler/mt.h"
#include "kernel/time.h"
#include "mem/mem.h"
#include "kernel/acpi.h"
#include "kernel/reg.h"
#include "kernel/smp.h"
#include "kernel/syscall.h"
#include "kernel/vfs.h"
#include "kernel/video.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


extern void keyboard_handler_c(void);
extern char __bss_start, __bss_end;


uint8_t kernel_stack[KERNEL_STACK_SIZE] __aligned(16);

void hcf(void) {
	for (;;) {
		__asm__ ("hlt");
	}
}

void delay(int count) {
	for (int i = 0; i < count; i++) {
		__asm__ volatile("outb %%al, $0x80" : : "a"(0));
	}
}

static void verify_boot_environment(void) {
	if (LIMINE_BASE_REVISION_SUPPORTED == false) hcf();
	if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) hcf();
	if (hhdm_request.response == NULL) hcf();
	if (memmap_request.response == NULL) hcf();
}

static void enable_sse(void) {
	uint64_t cr0,cr4;
	__asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 &= ~(1ULL << 2);
	cr0 |= (1ULL << 1);
	__asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

	__asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= (1ULL << 9);
	cr4 |= (1ULL << 10);
	__asm__ volatile("mov %0, %%cr4" :: "r"(cr4));

}

void kbd_init(void) {
    while (io_read_8(0x64) & 1) {
        io_read_8(0x60);
    }
}

void analyse_page_table(page_table_t *page_tbl, uint32_t level, uint32_t ident) {
	if (level == 2) {
		return;
	}
	for(uint64_t i = 0; i < 512; i++) {
		if (page_tbl->entries[i].present == 1) {
			for (uint32_t p = 0; p < ident; p++){
				printf(" ");
			}
			printf("[%d] set\n",i);
			analyse_page_table((page_table_t *)PHYS_TO_VIRT(page_tbl->entries[i].address  << 12), level-1, ident + 1);
		}
	}
}

static void analyse_mem() {
	printf("PML4:\n");
	analyse_page_table(kernel_pml4, 4, 0);
}



void kernel_initialize(void) {
	//memset(&__bss_start, 0, &__bss_end - &__bss_start);
	enable_sse();
	verify_boot_environment();
	cli();
	init_fb();

	init_mem(memmap_request.response);
	vfs_init();
	acpi_init();
	init_gdt();
	init_gdt_local();
	tss_set_kernel_stack((uint64_t)kernel_stack + KERNEL_STACK_SIZE);
	
	init_smp_data();

	intr_init();
	init_syscall();
	init_multitasking();
	printf("multitasking enabled\n");
	kbd_init();
	init_rtc();
	init_pci();
	//smp_init();
	//asm volatile("cli; hlt");
	printf("KERNEL INIT DONE!\n");
}