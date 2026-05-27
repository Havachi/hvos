#ifndef HVOS_MT_H
#define HVOS_MT_H

#include <stdint.h>
#include "kernel/intr.h"
#include "kernel/scheduler/task.h"
#include "mem/mem.h"



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


typedef struct {
    task_t *current;
    task_t *idle_task;
    uint64_t ready_task_count;
    task_t *ready_list;
    uint64_t min_vruntime
} cpu_task_list_t;


uint64_t scheduler_c(uint64_t old_rsp);
void init_multitasking();

task_t *create_user_task(void (*entry_point)(void), pt_entry *process_pml4, char *name);

void create_test_task();

void push_new_task(task_t *task);
task_t *get_current_task();
cpu_task_list_t *get_cpu_task_list();
static inline void yield(void) {
    asm volatile("int %0"::"i"(INT_SCHEDULER));
}

#endif
