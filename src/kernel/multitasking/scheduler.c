#include "kernel/gdt.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include <stdint.h>

#define MAX_VRUNTIME  200

extern void context_switch(uint64_t from_krsp, uint64_t to_krsp);
extern void switch_to_user(void *rip, void *rsp, uint64_t cs, uint64_t ds);
extern cpu_task_list_t **cpu_task_lists;

uint64_t scheduler_c(uint64_t old_rsp){
	cpu_task_list_t *cpu_list = get_cpu_task_list();
	task_t *current_task = get_current_task();

	if (cpu_list->ready_list == NULL) {
		return old_rsp;
	}

	current_task->k_rsp = (void *)old_rsp;
	task_t *new_task;
	if (current_task != NULL && current_task->next != NULL) {
		new_task = current_task->next;
	} else {
		if (cpu_list->ready_list != NULL)
			new_task = cpu_list->ready_list;
		else
			new_task = cpu_list->idle_task;
	}
	if (current_task->state != STATE_DEAD) {
		cpu_list->current->state = STATE_READY;
		cpu_list->ready_list = cpu_list->current;
	} else {
		if (current_task->next != NULL)
			current_task->state = STATE_WAITING;
		else
			cpu_list->current = NULL;
	}	

	cpu_list->current = new_task;
	new_task->state = STATE_RUNNING;
	
	tss_entry_t *local_tss = get_local_tss();
	local_tss->rsp0 = (uint64_t)new_task->kernel_stack_base;

	if (new_task->cr3 != 0) {
		__asm__ __volatile("movq %0, %%cr3"::"r"(new_task->cr3));
	}

	return (uint64_t)new_task->k_rsp;
}
