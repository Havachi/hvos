#include "asm/segment.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include "mem/mem.h"
#include <kernel/scheduler/thread.h>
#include <stdint.h>

safe_lock_t next_tid_lock = {.locked = 0}; 
static uint64_t next_tid = 0;

static uint64_t get_next_tid() {
	uint64_t flags = safe_lock(&next_tid_lock);
	uint64_t ret = next_tid;
	next_tid += 1;
	safe_unlock(&next_tid_lock, flags);
	return ret;
}

static void push_thread(thread_t *list, thread_t *new) {
	thread_t *head = list;
	if (list == NULL) {
		list = new;
		return;
	}
	while(head->next != NULL)
		head = head->next;
	head->next = new;
}

thread_t *new_thread() {
	thread_t *t = kzalloc(sizeof(thread_t));
	if (!t) {
		return NULL;
	}
	t->tid = get_next_tid();
	return t;
}

thread_t *new_elf_thread(uint64_t rip, uint64_t ustack, uint64_t kstack) {
	thread_t *t = new_thread();
	if (!t) return NULL;
	uint64_t cs = __USER_CS | 3;
	uint64_t ds = __USER_DS | 3;

	uint8_t *krsp = (uint8_t *) kstack;
	krsp -= sizeof(new_task_kernel_stack_t);
	new_task_kernel_stack_t *stack = (new_task_kernel_stack_t *)krsp;
	memset(stack, 0, sizeof(new_task_kernel_stack_t));
	stack->rip = rip;
	stack->cs = cs;
	stack->rflags = 0x202;
	stack->usermode_rsp = ustack;
	stack->usermode_ss = ds;

	t->k_rsp = (void *)(uint64_t)krsp;
	t->kernel_stack_base = (void *)kstack;
	return t;
}

thread_t *new_kernel_thread(uint64_t rip, uint64_t kstack) {
	thread_t *t = new_thread();
	if (!t) return NULL;

	uint8_t *krsp = (uint8_t *) kstack;
	krsp -= sizeof(new_task_kernel_stack_t);
	new_task_kernel_stack_t *stack = (new_task_kernel_stack_t *)krsp;
	memset(stack, 0, sizeof(new_task_kernel_stack_t));

	stack->rip = rip;
	stack->cs = __KERNEL_CS;
	stack->rflags = 0x202;
	stack->usermode_rsp = (uint64_t)kstack;
	stack->usermode_ss = __KERNEL_DS;

	t->k_rsp = (void *)(uint64_t) krsp;
	t->kernel_stack_base = (void *)kstack;
	t->state = STATE_READY;
	t->block_channel = NULL;
	return t;
}


//Push the new thread in all the thread list
void push_new_thread(thread_t *s) {
	cpu_task_list_t *cpu = get_cpu_task_list();
	list_push(cpu->thread_list, (void*) s);
}



void remove_thread_from_process(thread_t *s) {
	if (s->process == NULL) {
		return;
	}
	thread_t *prev = NULL;
	thread_t *head = s->process->primary;
	thread_t *next = s->process->primary->next;

	while (head->next != NULL){
		if (head->tid == s->tid) {
			break;
		}
		prev = head;
		head = next;
		next = head->next;
	}

	if (head->tid != s->tid) {
		return;
	}

	if (head != NULL) {
		if (next != NULL && prev != NULL) {
			prev->next = head->next;
		} else if ( prev != NULL) {
			prev->next = NULL;
		}
	}
}

void free_thread(thread_t *t) {
	remove_thread_from_process(t);
	kfree(t);
}


int _cb_thread_by_tid(void *t_raw, void *tid_raw) {
	uint64_t tid = (uint64_t)(uintptr_t)tid_raw;
	thread_t *thread = (thread_t *)t_raw;
	return (thread->tid == tid);
}

thread_t *thread_by_tid (uint64_t tid) {
	list_t *thread_list = get_cpu_task_list()->thread_list;
	return (thread_t *)get_first_match(thread_list, _cb_thread_by_tid, (void *)(uintptr_t)tid);
}


bool isdead(thread_t *t){
	return (t && t->state == STATE_DEAD);
}

bool isalive(thread_t *t){
	return !isdead(t);
}