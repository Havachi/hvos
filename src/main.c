#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "kernel/boot.h"
#include "kernel/time.h"
#include "kernel/video.h"
#include "kernel/elf.h"

extern long (*backend_io_write)(int fd, const char *buf, size_t count);
extern long console_write(void *file, const char *buf, size_t count, uint64_t *offset);

static long kernel_write_adapter(int fd, const char *buf, size_t count) {
    if (fd == 1 || fd == 2) {
        uint64_t dummy = 0;
        return console_write(NULL, buf, count, &dummy);
    }
    return -1;
}

struct limine_framebuffer *fb = NULL;
extern void kernel_initialize(void);
extern uint64_t total_pages;

extern uint32_t g_acpi_cpu_count;

void print_available_ram(uint64_t total_bytes) {
    uint64_t mib = total_bytes / (1024 * 1024);
    uint64_t gib = mib / 1024;

    if (gib > 0) {
        printf("Available RAM: %d GB\n", gib);
    } else {
        printf("Available RAM: %d MB\n", mib);
    }
}

void kmain(void) {

    backend_io_write = kernel_write_adapter;
	fb = framebuffer_request.response->framebuffers[0];
	kernel_initialize();
    clear_screen();
	printf("Term: %dx%d\n", fb->width, fb->height);
	uint64_t ram_bytes = total_pages * 4096;
	print_available_ram(ram_bytes);
    printf("CPU with %d core\n", g_acpi_cpu_count);
    printf("%s\n", datetime_to_str(now()));
    printf("loading shell\n");


	elf_load_and_run("/init.elf");
	//create_test_task();
	abort();
}
