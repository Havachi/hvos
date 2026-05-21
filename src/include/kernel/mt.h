#ifndef HVOS_MT_H
#define HVOS_MT_H

#include <stdint.h>
#include "mem/mem.h"
#include "kernel/scheduler/task_state.h"

typedef struct {
	char name[64];
	uint64_t rsp;
	uint64_t kernel_stack_top;
	uint64_t cr3;
	uint32_t state;
	int exit_code;
} task_t;

typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) stack_frame_t;

uint64_t scheduler_c(uint64_t old_rsp);
void init_multitasking();

task_t *create_user_task(void (*entry_point)(void), pt_entry *process_pml4, char *name);

void create_test_task();

static inline void yield(void) {
    asm volatile("int $0x30");
}

#endif
