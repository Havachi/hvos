#include "kernel/gdt.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/smp.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VRUNTIME  200

extern cpu_task_list_t **cpu_task_lists;
extern uint64_t syscall_kernel_stack;
thread_t *get_next_thread() {

	thread_t *next = NULL;

#if defined (SCH_ALG) && SCH_ALG == SCH_ALG_CFS
//CFS
	next = get_next_thread_cfs();
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
		if (t && strcmp(t->name, "IDLE_THREAD") == 0) {
			return t;
		}
		curr = curr->next;
	}

	return NULL;
}

extern void switch_to(void *old_rsp_ptr, void *next_thread_ptr);


#define PTE_PRESENT  (1ULL << 0)
#define PTE_WRITABLE (1ULL << 1)
#define PTE_USER     (1ULL << 2)

void debug_dump_page_entry(uint64_t pml4_phys, uint64_t vaddr) {
	pml4_table_t *pml4 = (pml4_table_t *)PHYS_TO_VIRT(pml4_phys);
	
	uint64_t pml4_idx = (vaddr >> 39) & 0x1FF;
	uint64_t pdpt_idx = (vaddr >> 30) & 0x1FF;
	uint64_t pd_idx   = (vaddr >> 21) & 0x1FF;
	uint64_t pt_idx   = (vaddr >> 12) & 0x1FF;

	printf("[DEBUG] Mapping for 0x%llx in CR3 0x%llx:\n", vaddr, pml4_phys);

	uint64_t pml4E = ((uint64_t*)pml4)[pml4_idx];
	printf("  PML4[%d]: 0x%llx (Pres: %d, User: %d)\n", pml4_idx, pml4E, !!(pml4E & PTE_PRESENT), !!(pml4E & PTE_USER));
	if (!(pml4E & PTE_PRESENT)) return;

	uint64_t *pdpt = (uint64_t*)PHYS_TO_VIRT(pml4E & ~0xFFFULL);
	uint64_t pdptE = pdpt[pdpt_idx];
	printf("  PDPT[%d]: 0x%llx (Pres: %d, User: %d)\n", pdpt_idx, pdptE, !!(pdptE & PTE_PRESENT), !!(pdptE & PTE_USER));
	if (!(pdptE & PTE_PRESENT)) return;

	uint64_t *pd = (uint64_t*)PHYS_TO_VIRT(pdptE & ~0xFFFULL);
	uint64_t pdE = pd[pd_idx];
	printf("  PD[%d]:   0x%llx (Pres: %d, User: %d)\n", pd_idx, pdE, !!(pdE & PTE_PRESENT), !!(pdE & PTE_USER));
	if (!(pdE & PTE_PRESENT)) return;

	uint64_t *pt = (uint64_t*)PHYS_TO_VIRT(pdE & ~0xFFFULL);
	uint64_t ptE = pt[pt_idx];
	printf("  PT[%d]:   0x%llx (Pres: %d, User: %d)\n", pt_idx, ptE, !!(ptE & PTE_PRESENT), !!(ptE & PTE_USER));
}

void switch_tss_stack(thread_t *next_thread) {
	tss_entry_t *local_tss = get_local_tss();

	local_tss->rsp0 = (uint64_t)next_thread->kernel_stack_base;
}

uint64_t schedule(uint64_t old_rsp, uint64_t from) {
	cpu_task_list_t *cpu_list = get_cpu_task_list();
	cpu_data_t *cpu_data = get_current_cpu_data();
	thread_t *current_thread = cpu_list->current_thread;

	if (from == 1) {
		return old_rsp;
	}

	if (current_thread == NULL) return old_rsp;

	current_thread->k_rsp = (void *)old_rsp;


	thread_t *next_thread = get_next_thread();

	if (next_thread == NULL) {
		if (strcmp(current_thread->name, "IDLE_THREAD") == 0) {
			return old_rsp;
		} else if (cpu_list->thread_list->count == 2) {
			return old_rsp;
		}
		next_thread = find_idle_thread(cpu_list->thread_list);
		if (next_thread == NULL)
			return old_rsp;
	}
	if (current_thread == next_thread) {
		current_thread->state = STATE_RUNNING;
		return (uint64_t)current_thread->k_rsp;
	}

	if (current_thread->state == STATE_RUNNING) {
		current_thread->state = STATE_READY;
	}
	next_thread->state = STATE_RUNNING;
	cpu_list->current_thread = next_thread;
	
	switch_tss_stack(next_thread);

	cpu_data->stack_top = (uint64_t)next_thread->kernel_stack_base;

	uint64_t kernel_cr3 = VIRT_TO_PHYS(kernel_pml4);
	if (next_thread->process->cr3 != kernel_cr3) {
		pml4_table_t *next_pml4 = (pml4_table_t *)PHYS_TO_VIRT(next_thread->process->cr3);
		for (int i = 256; i < 512; i++) {
			next_pml4->entries[i] = kernel_pml4->entries[i];
		}
	}

	if (current_thread->process->cr3 != next_thread->process->cr3) {
		__asm__ __volatile__("movq %0, %%cr3" :: "r"(next_thread->process->cr3): "memory");
	}
	//printf("[SCHED] %d->%d\n", current_thread->tid, next_thread->tid);
	//printf("[SCHED] %016p -> %016p\n", old_rsp, next_thread->k_rsp);

	return (uint64_t)next_thread->k_rsp;
}
void chk_rsp(){
	uint64_t cu_rsp;
	__asm__ volatile("mov %%rsp, %0" : "=r"(cu_rsp));

	if (cu_rsp >= 0xffffa00000800000 && cu_rsp <= 0xffffa00000900000) {
		printf("\n\nMEGAFUN! RSP: %p\n", cu_rsp);
	}
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
	for (uint64_t i = 0; i < cpu->thread_list->count; i++) {
		thread_t *t = list_get_at(cpu->thread_list, i);
		if (t->state == STATE_WAITING && t->block_channel == channel) {
			t->state = STATE_READY;
			t->block_channel = NULL;
			break;
		}
	}
}


void update_sleeping_queue() {
	cpu_task_list_t *cpu = get_cpu_task_list();
	delta_queue_entry_t *top = NULL;
	if (!cpu->sleeping_queue || cpu->sleeping_queue->count == 0)
		return;
	top = list_get_at(cpu->sleeping_queue, 0);
	if (top == NULL) {
		return;
	}
	if (top->delta == 0){
		for (uint64_t i = 0; i < cpu->thread_list->count; i++) {
			thread_t *t = list_get_at(cpu->thread_list, i);
			if (t && t->tid == top->tid) {
				t->state = STATE_READY;
			}
		}
		list_pop(cpu->sleeping_queue);
	} else {
		top->delta--;
	}
}

uint64_t get_total_delta(){ 
	cpu_task_list_t *cpu = get_cpu_task_list();

	uint64_t total_delta = 0;
	for (uint64_t i = 0; i < cpu->sleeping_queue->count; i++) {
		delta_queue_entry_t *dt = (delta_queue_entry_t *)list_get_at(cpu->sleeping_queue, i);
		total_delta += dt->delta;
	}
	return total_delta;
}

void ksleep(uint64_t s) {
	kusleep(s*1000);
}

void kusleep(uint64_t ms) {
	cpu_task_list_t *cpu = get_cpu_task_list();
	cpu->current_thread->state = STATE_SLEEPING;
	delta_queue_entry_t *dt = kmalloc(sizeof(delta_queue_entry_t));
	dt->delta = (ms - get_total_delta());
	dt->tid = cpu->current_thread->tid;
	list_push(cpu->sleeping_queue, (void*)dt);
	kernel_yield();
}