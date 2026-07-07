
#include "drivers/console.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include "mem/mem.h"
#include <fcntl.h>
#include <kernel/scheduler/process.h>
#include <kernel/scheduler/thread.h>
#include <stdint.h>

safe_lock_t next_pid_lock = {.locked = 0}; 
static uint64_t next_pid = 0;

static uint64_t get_next_pid() {
	uint64_t flags = safe_lock(&next_pid_lock);
	uint64_t ret = next_pid;
	next_pid += 1;
	safe_unlock(&next_pid_lock, flags);
	return ret;
}

process_t *new_process() {
	process_t *p = kzalloc(sizeof(process_t));
	if(!p){
		return NULL;
	}
	p->pid = get_next_pid();
	p->cr3 = 0;
	p->heap_end = 0;
	p->parent = NULL;
	p->exit_code = 0;
	for (int i = 0; i < MAX_FILES_PER_PROCESS; i++) {
		p->file_table[i] = NULL;
	}
	p->file_table[0] = create_kernel_console_file(O_RDONLY);
	p->file_table[1] = create_kernel_console_file(O_WRONLY);
	p->file_table[2] = create_kernel_console_file(O_WRONLY);
	p->primary = NULL;
	return p;
}


process_t *new_elf_process(uint64_t rip, uint64_t ustack, uint64_t kstack) {
	process_t *p = new_process();
	if (!p) return NULL;
	thread_t *t = new_elf_thread(rip, ustack, kstack);	
	if (!t) {
		free_process(p);
		return NULL;
	}

	t->block_channel = NULL;
	t->vruntime = 0;
	t->state = STATE_READY;
	t->next = NULL;
	t->process = p;
	p->primary = t;
	
	push_new_thread(t);
	push_new_process(p);
	return p;
}

void free_process(process_t *p) {
	kfree(p);
}

void push_new_process(process_t *p) {
	cpu_task_list_t *cpu = get_cpu_task_list();
	list_push(cpu->process_list, (void *)p);
}

int _cb_process_by_pid(void *t_raw, void *pid_raw) {
	uint64_t pid = (uint64_t)(uintptr_t)pid_raw;
	process_t *p = (process_t *)t_raw;
	return (p->pid == pid);
}

process_t *process_by_pid (uint64_t pid) {
	list_t *process_list = get_cpu_task_list()->process_list;
	return (process_t *)get_first_match(process_list, _cb_process_by_pid, (void *)(uintptr_t)pid);
}


process_t *clone_process(process_t *process) {
	process_t *child = new_process();
	child->parent = process;
}

