#ifndef HVOS_MT_H
#define HVOS_MT_H

#include <stdint.h>
#include "kernel/intr.h"
#include "kernel/scheduler/task.h"
#include "kernel/scheduler/thread.h"
#include "mem/mem.h"
#include "process.h"



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
    uint64_t usermode_rsp;
    uint64_t usermode_ss;
} __attribute__((packed)) new_task_kernel_stack_t;

typedef struct {
    uint64_t min_vruntime;
    //Will replace the ready_list
    uint64_t thread_count;
    list_t *process_list;
    list_t *thread_list;
    thread_t *current_thread;
} cpu_task_list_t;


uint64_t scheduler_c(uint64_t old_rsp);
void schedule(void);
void init_multitasking();

task_t *create_user_task(void (*entry_point)(void), pt_entry *process_pml4, char *name);

void create_test_task();
task_t *new_user_task(uint64_t rip, uint64_t user_stack, uint64_t kernel_stack);
void push_new_task(task_t *task);
process_t *get_current_process();
cpu_task_list_t *get_cpu_task_list();
static inline void yield(void) {
    __asm__ volatile("int %0"::"i"(INT_SCHEDULER));
}


#define SCH_ALG_RR  0x01
#define SCH_ALG_CFS 0x02

#define SCH_ALG SCH_ALG_RR

thread_t *get_next_thread();
thread_t *get_next_thread_rr();
thread_t *get_next_thread_cfs();
thread_t *find_idle_thread(list_t *thread_list);

void notify_wait_channel(void *channel);

#endif
