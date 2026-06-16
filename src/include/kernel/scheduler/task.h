#ifndef HVOS_SCHEDULER_TASK_H
#define HVOS_SCHEDULER_TASK_H

#include "hvos/compiler_attributes.h"
#include "kernel/vfs.h"
#include <stdint.h>

#ifndef MAX_FILES_PER_PROCESS
#define MAX_FILES_PER_PROCESS 32
#endif


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
    uint64_t heap_end;
    struct task_s *parent;
    struct task_s *next;
    int exit_code;
    file_t *file_table[MAX_FILES_PER_PROCESS];
} __packed task_t;

task_t *new_idle_task();
task_t *new_task();

task_t *task_by_pid(uint64_t pid);

#endif