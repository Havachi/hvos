#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kernel/boot.h"
#include "kernel/time.h"
#include "kernel/video.h"
#include "kernel/elf.h"
#include "klibc/printf.h"

struct limine_framebuffer *fb = NULL;
extern void kernel_initialize(void);
extern uint64_t total_pages;

extern uint32_t g_acpi_cpu_count;

void print_available_ram(uint64_t total_bytes) {
    uint64_t mib = total_bytes / (1024 * 1024);
    uint64_t gib = mib / 1024;

    if (gib > 0) {
        // Formats as e.g., "Available RAM: 4 GB, 256 MB"
        kprintf("Available RAM: %d GB\n", gib);
    } else {
        kprintf("Available RAM: %d MB\n", mib);
    }
}

void kmain(void) {

	fb = framebuffer_request.response->framebuffers[0];
	kernel_initialize();
    clear_screen();
	print("Welcome to HVOS v0.1\n");
	kprintf("Term: %dx%d\n", fb->width, fb->height);
	uint64_t ram_bytes = total_pages * 4096;
	print_available_ram(ram_bytes);
    kprintf("CPU with %d core\n", g_acpi_cpu_count);
    kprintf("%s\n", datetime_to_str(now()));
    kprintf("loading shell\n");


	elf_load_and_run("shell.elf");
	//create_test_task();

	hcf();
}
