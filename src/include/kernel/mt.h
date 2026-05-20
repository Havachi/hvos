#ifndef HVOS_MT_H
#define HVOS_MT_H

#include <stdint.h>
#include "kernel/acpi.h"

#define TASK_STATE_RUNNING	0
#define TASK_STATE_WAITING	1
#define TASK_STATE_DEAD		2

#define TASK_STATE_BLOCKED_ON_KEYBOARD 10

typedef struct {
	uint64_t rsp;
	uint64_t kernel_stack_top;
	uint64_t cr3;
	uint32_t state;
	int exit_code;
} task_t;

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) stack_frame_t;

uint64_t scheduler_c(uint64_t old_rsp);
void init_multitasking();

task_t *create_user_task(void (*entry_point)(void), pt_entry *process_pml4);

static inline void yield(void) {
    asm volatile("int $0x30");
}

#endif
