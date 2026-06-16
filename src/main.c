#include "include/cpu/io.h"
#include "include/kernel/acpi.h"
#include "include/kernel/scheduler/mt.h"
#include "include/kernel/scheduler/task.h"
#include "kernel/scheduler/task_state.h"
#include <stddef.h>
#include <stdint.h>
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
	uint64_t ram_bytes = total_pages * 4096;
	print_available_ram(ram_bytes);
    printf("CPU with %d core\n", g_acpi_cpu_count);
    printf("%s\n", datetime_to_str(now()));
    printf("loading shell\n");

	int pid = elf_load_and_run("/shell.elf");
    
	//create_test_task();
    while (task_by_pid(pid) != NULL && task_by_pid(pid)->state != STATE_DEAD){
        
        __asm__ __volatile("hlt");
    }
    printf("rebooting...");
	reboot();
    abort();
}
