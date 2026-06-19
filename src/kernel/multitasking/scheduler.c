#include "kernel/gdt.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_VRUNTIME  200

extern cpu_task_list_t **cpu_task_lists;

thread_t *get_next_thread() {

	thread_t *next = NULL;

#if defined (SCH_ALG) && SCH_ALG == SCH_ALG_CFS
//CFS
#else
//Round robin
	next = get_next_thread_rr();
#endif
	return next;
}

thread_t *find_idle_thread(list_t *thread_list) {
	list_node_t *curr = thread_list->head;
	while(curr != NULL) {
		thread_t *t = (thread_t *)curr->data;
		if (strcmp(t->name, "IDLE_THREAD") == 0) {
			return t;
		}
		curr = curr->next;
	}

	return NULL;
}

extern void switch_to(void *old_rsp_ptr, void *next_thread_ptr);

void schedule(void) {
    cpu_task_list_t *cpu_list = get_cpu_task_list();
    thread_t *current_thread = cpu_list->current_thread;

    if (current_thread == NULL) return;

    thread_t *next_thread = get_next_thread();
    if (next_thread == NULL) {
        if (strcmp(current_thread->name, "IDLE_THREAD") == 0) {
            return;
        }
        next_thread = find_idle_thread(cpu_list->thread_list);
    }

    if (current_thread == next_thread) {
        current_thread->state = STATE_RUNNING;
        return;
    }

    if (current_thread->state == STATE_RUNNING) {
        current_thread->state = STATE_READY;
    }
    next_thread->state = STATE_RUNNING;
    cpu_list->current_thread = next_thread;

    tss_entry_t *local_tss = get_local_tss();
    local_tss->rsp0 = (uint64_t)next_thread->kernel_stack_base;

    if (current_thread->process->cr3 != next_thread->process->cr3) {
        __asm__ __volatile__("movq %0, %%cr3" :: "r"(next_thread->process->cr3));
    }
    switch_to(&current_thread->k_rsp, next_thread);
}

uint64_t scheduler_c(uint64_t old_rsp) {
    cpu_task_list_t *cpu_list = get_cpu_task_list();
    thread_t *current_thread = cpu_list->current_thread;

    if (current_thread == NULL) {
        return old_rsp;
    }

    thread_t *next_thread = get_next_thread();

    if (next_thread == NULL) {
        if (strcmp(current_thread->name, "IDLE_THREAD") == 0) {
            return old_rsp;
        }
        next_thread = find_idle_thread(cpu_list->thread_list);
    }

    if (current_thread == next_thread && current_thread->state != STATE_DEAD) {
        current_thread->state = STATE_RUNNING;
        return old_rsp;
    }

    current_thread->k_rsp = (void *)old_rsp;

    if (current_thread->state == STATE_RUNNING) {
        current_thread->state = STATE_READY;
    }

	if (current_thread->state == STATE_DEAD) {
		printf("TID %d is dead\n", current_thread->tid);
		//next_thread = current_thread->process->parent->primary;
	}

    next_thread->state = STATE_RUNNING;
    cpu_list->current_thread = next_thread;

    tss_entry_t *local_tss = get_local_tss();
    local_tss->rsp0 = (uint64_t)next_thread->kernel_stack_base;

    if (current_thread->process->cr3 != next_thread->process->cr3) {
		if (next_thread->process->cr3 == 0) {
			printf("!!! PANIC !!!\n");
			printf("Tried to load 0 as cr3\n");
			while(1) {__asm__ __volatile("hlt");}
		}
        __asm__ __volatile__("movq %0, %%cr3" :: "r"(next_thread->process->cr3));
    }

    return (uint64_t)next_thread->k_rsp;
}

void update_curr_thread(void) {
	cpu_task_list_t *cpu_list = get_cpu_task_list();
	thread_t *current_thread = cpu_list->current_thread;
	if (current_thread == NULL) {
		return;
	}

	current_thread->vruntime++;

	if (current_thread->sleep_ticks > 0) {
		current_thread->sleep_ticks--;
	}
}

void notify_wait_channel(void *channel) {
	if (channel == NULL) return;
	cpu_task_list_t *cpu = get_cpu_task_list();
	list_node_t *curr = cpu->thread_list->head;

	while (curr != NULL) {
		thread_t *t = (thread_t *)curr->data;
		if (t->state == STATE_WAITING && t->block_channel == channel) {
			t->state = STATE_READY;
			t->block_channel = NULL;
			break;
		}
		curr = curr->next;
	}
}