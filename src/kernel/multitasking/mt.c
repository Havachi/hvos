#include "kernel/scheduler/mt.h"
#include "kernel/acpi.h"
#include "kernel/local_apic.h"
#include "kernel/scheduler/task_state.h"

cpu_task_list_t **cpu_task_lists;
task_t bsp_main_task;

void init_bsp(){
	cpu_task_list_t *bsp_task_list = cpu_task_lists[local_apic_get_id()];
	bsp_main_task.k_rsp = NULL;
	bsp_main_task.vruntime = 0;
	bsp_main_task.state = STATE_RUNNING;
	bsp_main_task.next = NULL;
	bsp_task_list->current = &bsp_main_task;
}

void init_ap(){

}

void push_new_task(task_t *task) {
	cpu_task_list_t *current_cpu_list = get_cpu_task_list();
	if (current_cpu_list->ready_list != NULL){
		current_cpu_list->ready_list->next = task;
	} else {
		current_cpu_list->ready_list = task;
	}
	current_cpu_list->ready_task_count++;
}

void init_multitasking(){
	uint32_t cpu_count = g_acpi_cpu_count;
	cpu_task_lists = (cpu_task_list_t **)kmalloc(sizeof(cpu_task_list_t) * cpu_count);

	for (uint32_t i = 0; i < cpu_count; i++)
		cpu_task_lists[i] = kmalloc(sizeof(cpu_task_list_t));
	cpu_task_list_t *bsp_task_list = get_cpu_task_list();
	bsp_task_list->ready_list = NULL;
	bsp_task_list->min_vruntime = 0;
	bsp_task_list->ready_task_count = 0;
	init_bsp();

	bsp_task_list->idle_task = new_idle_task();
}

cpu_task_list_t *get_cpu_task_list() {
	return cpu_task_lists[local_apic_get_id()];
}

task_t *get_current_task() {
	cpu_task_list_t *task_list = get_cpu_task_list();
	return task_list->current;
}