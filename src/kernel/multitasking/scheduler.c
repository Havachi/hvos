#include "kernel/gdt.h"
#include "kernel/scheduler/mt.h"
#include "klibc/printf.h"
#include "klibc/string.h"
#include <stdint.h>

#define MAX_VRUNTIME  200

extern void context_switch(uint64_t from_krsp, uint64_t to_krsp);
extern void switch_to_user(void *rip, void *rsp, uint64_t cs, uint64_t ds);
extern cpu_task_list_t **cpu_task_lists;

void switch_task(task_t *new_task) {
	cpu_task_list_t *local_task_list = get_cpu_task_list();
	task_t *current_task = local_task_list->current;
	tss_entry_t *local_tss = get_local_tss();
	task_t *old = current_task;
	current_task = new_task;

	local_tss->rsp0 = (uint64_t)new_task->kernel_stack_base;
	context_switch((uint64_t)&old->k_rsp, (uint64_t)new_task->k_rsp);
	if (new_task->cr3 != 0) {
		asm volatile("movq %0, %%cr3"::"r"(new_task->cr3));
	}
}

uint64_t scheduler_c(uint64_t old_rsp){
	cpu_task_list_t *cpu_list = get_cpu_task_list();
	task_t *current_task = get_current_task();

	if (cpu_list->ready_list == NULL) {
		return old_rsp;
	}

	current_task->k_rsp = (void *)old_rsp;

	task_t *new_task = cpu_list->ready_list;
	cpu_list->current = new_task;

	tss_entry_t *local_tss = get_local_tss();
	local_tss->rsp0 = (uint64_t)new_task->kernel_stack_base;

	if (new_task->cr3 != 0) {
		asm volatile("movq %0, %%cr3"::"r"(new_task->cr3));
	}

	return (uint64_t)new_task->k_rsp;
	while(1) {
		if (cpu_list->ready_list == NULL){
			//switch_task(cpu_list->idle_task);
			//context_switch(cpu_list->idle_task->k_rsp, cpu_list->current->k_rsp, cpu_list->current->cr3);
			break;
		} else {
			get_current_task()->k_rsp = (void *)old_rsp;
			switch_task(cpu_list->ready_list);
		}
	}
	return old_rsp;
}
