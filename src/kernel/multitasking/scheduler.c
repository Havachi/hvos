#include "kernel/scheduler/mt.h"
#include "klibc/printf.h"
#include "klibc/string.h"

#define MAX_VRUNTIME  200

extern void context_switch(void* old_rsp, void* new_rsp, uint64_t cr3);
extern cpu_task_list_t **cpu_task_lists;

static void swap_tasks(task_t *task_a, task_t *task_b) {
	task_t *temp = task_a;
	task_a = task_b;
	task_b = temp;
}

uint64_t scheduler_c(uint64_t old_rsp){
	cpu_task_list_t *cpu_list = get_cpu_task_list();
	kprintf("start scheduling\n");
	while(1) {
		if (cpu_list->ready_list == NULL){
			swap_tasks(cpu_list->current, cpu_list->idle_task);
			//context_switch(cpu_list->idle_task->k_rsp, cpu_list->current->k_rsp, cpu_list->current->cr3);
			break;
		} else {
			swap_tasks(cpu_list->current, cpu_list->ready_list);
			context_switch(cpu_list->ready_list->k_rsp, cpu_list->current->k_rsp, cpu_list->current->cr3);
		}
	}

	return old_rsp;
}