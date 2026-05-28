#ifndef HVOS_SCHEDULER_TASK_H
#define HVOS_SCHEDULER_TASK_H

#include "hvos/compiler_attributes.h"
#include <stdint.h>

typedef struct task_s {
    uint64_t pid;
    void *k_rsp;
	uint64_t cr3;

    uint32_t state;
    uint32_t priority;
    uint64_t vruntime;
    uint64_t sleep_ticks;
    void    *block_channel;

    uint64_t tid;
	char name[16];

    void *kernel_stack_base;
    struct task_s *parent;
    struct task_s *next;
} __packed task_t;

task_t *new_idle_task();
task_t *new_task();

#endif