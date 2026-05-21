#include "kernel/mt.h"
#include "klibc/printf.h"
#include "klibc/string.h"

task_t tasks[3];
int current_task = 0;
int nb_tasks = 0;

static uint8_t idle_task_stack[PAGE_SIZE] __attribute__((aligned(16)));

extern void tss_set_kernel_stack(uint64_t stack_ptr);

void idle_task_entry(void) {
	for (;;) {
		asm volatile("hlt");
	}
}

uint64_t scheduler_c(uint64_t old_rsp) {
	if (tasks[current_task].state == TASK_STATE_RUNNING) {
		tasks[current_task].rsp = old_rsp;
	}

	int starting_task = current_task;
	while (1)
	{
		current_task = (current_task + 1) % nb_tasks;
		if (tasks[current_task].state == TASK_STATE_RUNNING){
			break;
		}

		if (tasks[current_task].state == TASK_STATE_WAITING){
			if (tasks[current_task].kernel_stack_top != 0){
				uint64_t k_bottom = tasks[current_task].kernel_stack_top - PAGE_SIZE;
				kfree((void *)k_bottom);
				tasks[current_task].kernel_stack_top = 0;
			}
			tasks[current_task].state = 4;
		}
		if (current_task == starting_task) {
			if (tasks[0].state == TASK_STATE_RUNNING) {
				current_task = 0;
				break;
			} else {
				kprintf("[KERNEL PANIC] No active thread running");
				asm volatile("cli; hlt");

			}
		}
	}

	if (tasks[current_task].cr3 != 0) {
		asm volatile("mov %0, %%cr3" :: "r"(tasks[current_task].cr3) : "memory");
	} else {
		asm volatile("mov %0, %%cr3" :: "r"(kernel_pml4_phys) : "memory");
	}

	
	uint64_t safe_kernel_stack = tasks[current_task].kernel_stack_top;
	tss_set_kernel_stack(safe_kernel_stack);
	return tasks[current_task].rsp;
}

task_t *create_task(void (*entry_point)(void)) {
	if (nb_tasks >= 3) return NULL;

	task_t *task = &tasks[nb_tasks];
	uint64_t *stack_bottom = kmalloc(4096);
	uint64_t *stack_top = (uint64_t *)((uintptr_t) stack_bottom + 4096);

	stack_frame_t *frame = (stack_frame_t *)((uintptr_t) stack_top - sizeof(stack_frame_t));
	kmemset(frame, 0, sizeof(stack_frame_t));

	frame->rip = (uint64_t) entry_point;
	frame->cs = 0x08;
	frame->ss = 0x10;
	frame->rsp = (uint64_t) stack_top;
	frame->rflags = 0x202;

	task->rsp = (uint64_t)frame;
	task->cr3 = kernel_pml4_phys;
	task->state = TASK_STATE_RUNNING;
	task->kernel_stack_top = (uint64_t)stack_top;
	nb_tasks++;
	return task;
}

void mt_main() {
	for (;;) {
		asm volatile("hlt");
	}
}

pt_entry* create_process_page_table(void) {
	uint64_t phys_addr =(uint64_t)pmm_alloc();
	pt_entry *process_pml4 = (pt_entry *)(phys_addr + hhdm_offset);

	kmemset(process_pml4, 0, PAGE_SIZE);

	for (int i = 256; i < 512; i++) {
		process_pml4[4] = current_pml4[i];
	}

	return process_pml4;
}

void init_multitasking() {
	//KERNEL TASK
	tasks[0].cr3 = kernel_pml4_phys;
	tasks[0].state = TASK_STATE_RUNNING;
	
	uint64_t *idle_stack_top = (uint64_t *)&idle_task_stack[4096];
	tasks[0].kernel_stack_top = (uint64_t) idle_stack_top;

	stack_frame_t *frame = (stack_frame_t *)((uintptr_t)idle_stack_top - sizeof(stack_frame_t));
    kmemset(frame, 0, sizeof(stack_frame_t));
    frame->rip = (uint64_t)idle_task_entry;
    frame->cs = 0x08;
    frame->ss = 0x10;
    frame->rsp = (uint64_t)idle_stack_top;
    frame->rflags = 0x202;

	tasks[0].rsp = (uint64_t)frame;

	current_task = 0;
	nb_tasks = 1;
}


task_t *create_user_task(void (*entry_point)(void), pt_entry *process_pml4) {
	if (nb_tasks >= 3) return NULL;

	task_t *task = &tasks[nb_tasks];

    uint64_t* k_stack_bottom = (uint64_t*)kmalloc(4096);
    task->kernel_stack_top = (uint64_t)k_stack_bottom + 4096;

	void * u_stack_phys = pmm_alloc();

	uint64_t user_stack_virt_top = 0x00007FFFFFFFF000;
	uint64_t user_stack_virt_base = user_stack_virt_top - PAGE_SIZE;

	vmm_map(process_pml4, user_stack_virt_base, (uint64_t) u_stack_phys,
		PTE_PRESENT | PTE_WRITABLE | PTE_USER);

	uint64_t hhdm_stack_top = (uint64_t) u_stack_phys + hhdm_offset + PAGE_SIZE;
	stack_frame_t *frame = (stack_frame_t *)((uintptr_t)hhdm_stack_top - sizeof(stack_frame_t));
	kmemset(frame, 0, sizeof(stack_frame_t));

	frame->rip = (uint64_t)entry_point;
	frame->rsp = (uint64_t)user_stack_virt_top;

	frame->cs = 0x1B;
	frame->ss = 0x23;
	frame->rflags = 0x202;

	uint64_t frame_offset_from_top = PAGE_SIZE - sizeof(stack_frame_t);
	task->rsp = ((uint64_t)u_stack_phys + hhdm_offset) + frame_offset_from_top;
	task->cr3 = (uint64_t)process_pml4;
	task->state = TASK_STATE_RUNNING;
	nb_tasks++;
	return task;
}


void unblock_tasks(uint32_t state) {
	for (int i = 0; i < nb_tasks; i++) {
		if (tasks[i].state == state){
			tasks[i].state = TASK_STATE_RUNNING;
		}
	}
}