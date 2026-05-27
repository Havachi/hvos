#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include "klibc/printf.h"
#include "klibc/string.h"

void kernel_idle_loop(void) {
	for(;;) {
		kprintf("idling\n");
		asm volatile("hlt");
	}
}

task_t *new_idle_task() {
	task_t *idle = kmalloc(sizeof(task_t));
	void *stack_raw = kmalloc(PAGE_SIZE);
	uint64_t *stack_top = (uint64_t *)((uint64_t)stack_raw + PAGE_SIZE);
	stack_top -= 15;
	stack_top--;
	*stack_top = (uint64_t)kernel_idle_loop;

	idle->k_rsp = stack_top;
	idle->state = STATE_READY;
	idle->next = NULL;
	idle->vruntime = 0;
	return  idle;
}

task_t *new_task() {
}

