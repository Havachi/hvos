#include "kernel/boot.h"
#include "kernel/elf.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/intr.h"
#include "kernel/isr.h"
#include "kernel/mem.h"
#include "kernel/acpi.h"
#include "kernel/mt.h"
#include "kernel/reg.h"
#include "kernel/smp.h"
#include "kernel/syscall.h"
#include "kernel/vfs.h"
#include "kernel/video.h"
#include "klibc/printf.h"

extern void keyboard_handler_c(void);
extern char __bss_start, __bss_end;

void hcf(void) {
	for (;;) {
		asm ("hlt");
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
	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 &= ~(1ULL << 2);
	cr0 |= (1ULL << 1);
	asm volatile("mov %0, %%cr0" :: "r"(cr0));

	asm volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= (1ULL << 9);
	cr4 |= (1ULL << 10);
	asm volatile("mov %0, %%cr4" :: "r"(cr4));

}

void kbd_init(void) {
    while (io_read_8(0x64) & 1) {
        io_read_8(0x60);
    }
}

void kernel_initialize(void) {
	//memset(&__bss_start, 0, &__bss_end - &__bss_start);
	//enable_sse();
	verify_boot_environment();
	cli();
	kbd_init();
	init_fb();
	init_mem(memmap_request.response, hhdm_request.response->offset);
	void * secure_tss_page = pmm_alloc();
	vfs_init();
	init_gdt(hhdm_offset, secure_tss_page);
	
	acpi_init();
	intr_init();
	init_syscall();
	init_multitasking();
	sti();
	
	smp_init();


	//asm volatile("cli; hlt");

	kprintf("KERNEL INIT DONE!\n");
}