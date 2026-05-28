#include "asm/segment.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/sync.h"
#include "klibc/printf.h"
#include "klibc/string.h"
#include <stdint.h>

safe_lock_t pid_lock = {.locked = 0};
static uint64_t next_pid = 0;

extern void new_task_setup();

void kernel_idle_loop(void) {
	for(;;) {
		kprintf("idling\n");
		asm volatile("hlt");
	}
}

task_t *new_idle_task() {
	
	
	void *stack_raw = pmm_alloc();
	map_page(kernel_pml4, PHYS_TO_VIRT(stack_raw), (uint64_t)stack_raw, PTE_WRITABLE | PTE_USER);

	task_t *idle = new_user_task((uint64_t)kernel_idle_loop, PHYS_TO_VIRT(stack_raw), PHYS_TO_VIRT(stack_raw));

	return  idle;
}

task_t *new_task() {
	

}

uint64_t new_pid() {
	uint64_t pid;
	uint64_t fl = safe_lock(&pid_lock);
	pid = next_pid++;
	safe_unlock(&pid_lock, fl);
	return pid;
}

task_t *new_user_task(uint64_t rip, uint64_t user_stack, uint64_t kernel_stack) {
	task_t *task = kmalloc(sizeof(task_t));
	if (!task) return NULL;
	uint64_t cs = __USER_CS | 3;
	uint64_t ds = __USER_DS | 3;

	uint8_t *krsp = (uint8_t *) kernel_stack;
	krsp -= sizeof(new_task_kernel_stack_t);
	new_task_kernel_stack_t *stack = (new_task_kernel_stack_t*)krsp;
	kmemset(stack, 0, sizeof(new_task_kernel_stack_t));
	stack->rbx = ds;
	stack->rip = rip;
	stack->cs = cs;
	stack->rflags = 0x202;
	stack->usermode_rsp = user_stack;
	stack->usermode_ss = ds;

	task->pid = new_pid();
	task->kernel_stack_base = (void *)kernel_stack;
	task->k_rsp = (void *)(uint64_t)krsp;
	return task;
}
