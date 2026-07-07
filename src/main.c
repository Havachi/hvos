#include "include/asm/asm.h"
#include "include/cpu/io.h"
#include "include/kernel/acpi.h"
#include "include/kernel/scheduler/process.h"
#include "include/kernel/scheduler/task.h"
#include "include/kernel/scheduler/thread.h"
#include "include/mem/mem.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/syscall.h"
#include <stdint.h>
#include <sys/wait.h>
#ifndef __KERNEL__
#define __KERNEL__ 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "kernel/boot.h"
#include "kernel/time.h"
#include "kernel/video.h"
#include "kernel/elf.h"

struct limine_framebuffer *fb = NULL;
extern void kernel_initialize(void);
extern uint64_t total_pages;

extern uint32_t g_acpi_cpu_count;
extern uint64_t memmap_get_total_pages(struct limine_memmap_response *memmap);

kernel_memmap_t *kmemmap = NULL;

void print_available_ram(uint64_t total_bytes) {
    uint64_t mib = total_bytes / (1024 * 1024);
    uint64_t gib = mib / 1024;

    if (gib > 0) {
        printf("Available RAM: %d GB\n", gib);
    } else {
        printf("Available RAM: %d MB\n", mib);
    }
}

void ps2_reboot() {
    uint8_t good = 0x02;
    while(good & 0x02)
        good = io_read_8(0x64);
    io_write_8(0x64, 0xFE);
    abort();
}

void reboot() {
    acpi_reboot();
    ps2_reboot();
    abort();
}

void kmain(void) {
	fb = framebuffer_request.response->framebuffers[0];
	kernel_initialize();
    clear_screen();

	printf("Term: %dx%d\n", fb->width, fb->height);
	uint64_t ram_bytes = memmap_get_total_pages(memmap_request.response) * PAGE_SIZE;
	print_available_ram(ram_bytes);
    printf("CPU with %d core\n", g_acpi_cpu_count);
    printf("%s\n", datetime_to_str(now()));
    printf("loading shell\n");
	int pid = execute_elf("/init.elf");
    printf("[KERNEL] INIT PID %d\n", pid);
    __asm__ __volatile__("sti");

    sys_waitpid(pid);
    printf("rebooting...");
	reboot();
    abort();
}
