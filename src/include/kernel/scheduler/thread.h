#ifndef _HVOS_SCHEDULER_THREAD_H
#define _HVOS_SCHEDULER_THREAD_H

#include "data_structure/list.h"
#include "kernel/scheduler/process.h"
#include <stdint.h>


typedef struct thread_s {
    uint64_t tid;
	char name[16];
    uint32_t state;
    void *k_rsp;
    void *kernel_stack_base;
    uint32_t priority;
    uint64_t vruntime;
    uint64_t sleep_ticks;
    void    *block_channel;
    process_t *process;
    struct thread_s *next;

}thread_t;

typedef list_t thread_list_t;

thread_t *new_thread();
thread_t *new_elf_thread(uint64_t rip, uint64_t ustack, uint64_t kstack);
thread_t *new_kernel_thread(uint64_t rip, uint64_t kstack);
void free_thread(thread_t *t);
void push_new_thread(process_t *p, thread_t *s);
thread_t *thread_by_tid (uint64_t tid);

bool isdead(thread_t *t);
bool isalive(thread_t *t);
#endif