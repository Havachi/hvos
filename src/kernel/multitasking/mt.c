#include "kernel/scheduler/mt.h"
#include "kernel/acpi.h"
#include "kernel/local_apic.h"
#include "kernel/scheduler/task_state.h"
#include <stdint.h>
#include <stdio.h>

cpu_task_list_t **cpu_task_lists;
task_t bsp_main_task;

void init_ap(){

}

void idle_loop() {
	while(1) {
		__asm__ __volatile__(
			"sti\n\t"
			"hlt\n\t"
			::
			: "memory"
		);
	}
}


void init_bsp_main_process() {
	cpu_task_list_t *bsp_task_list = get_cpu_task_list();
	process_t *bsp_main_process = new_process();
	bsp_main_process->cr3 = VIRT_TO_PHYS(kernel_pml4);

	thread_t *bsp_main_thread = new_thread();
	bsp_main_thread->state = STATE_RUNNING;
	bsp_main_thread->vruntime = 0;
	bsp_main_thread->next = NULL;
	sprintf(bsp_main_thread->name, "MAIN_THREAD\0");
	bsp_main_thread->process = bsp_main_process;
	bsp_main_thread->kernel_stack_base = NULL;
	uint64_t idle_stack_top = (uint64_t)kzalloc(4096) + 4096;
	thread_t *bsp_idle_thread = new_kernel_thread((uint64_t)idle_loop, idle_stack_top);

	bsp_idle_thread->state = STATE_READY;
	bsp_idle_thread->vruntime = 0;
	bsp_idle_thread->next = NULL;
	sprintf(bsp_idle_thread->name, "IDLE_THREAD");
	bsp_idle_thread->process = bsp_main_process;
	
	list_push(bsp_task_list->process_list, (void *)bsp_main_process);
	list_push(bsp_task_list->thread_list, (void *)bsp_main_thread);
	list_push(bsp_task_list->thread_list, (void *)bsp_idle_thread);

	bsp_main_process->primary = bsp_main_thread;
	bsp_task_list->current_thread = bsp_main_thread;
}

void init_multitasking(){



	uint32_t cpu_count = g_acpi_cpu_count;
	cpu_task_lists = (cpu_task_list_t **)kmalloc(sizeof(cpu_task_list_t) * cpu_count);

	for (uint32_t i = 0; i < cpu_count; i++) {
		cpu_task_lists[i] = kmalloc(sizeof(cpu_task_list_t));
		cpu_task_lists[i]->process_list = init_list();
		cpu_task_lists[i]->thread_list = init_list();
		cpu_task_lists[i]->current_thread = NULL;
	}
	cpu_task_list_t *bsp_task_list = get_cpu_task_list();
	//Setup BSP tasklist
	bsp_task_list->min_vruntime = 10;
	bsp_task_list->thread_count = 0;

	init_bsp_main_process();
	//bsp_task_list->idle_task = new_idle_task();
}

cpu_task_list_t *get_cpu_task_list() {
	return cpu_task_lists[local_apic_get_id()];
}

process_t *get_current_process() {
	cpu_task_list_t *task_list = get_cpu_task_list();
	/*
	if (task_list->thread_list->count == 1) {
		return (process_t *) task_list->process_list->head->data;
	}
	*/
	return task_list->current_thread->process;
}