#ifndef HVOS_PROCESS_H
#define HVOS_PROCESS_H

#include <stdint.h>
#include "kernel/reg.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/defs/list.h"
#include "mem/paging.h"

typedef uint64_t pid_t;



typedef struct context_s {
	uint64_t eax;
	uint64_t ecx;
	uint64_t edx;
	uint64_t ebx;
	uint64_t esp;
	uint64_t ebp;
	uint64_t esi;
	uint64_t edi;
	uint64_t eflags;
	uint64_t cr3;
    uint64_t eip;
} context_t;

typedef struct pcb_s {
	char		filename[512];
	context_t	regs;
	pid_t		pid;
	listnode_t*	self;
	void*		stack;
	uint64_t	state;
	uint64_t	time_slice;
	pd_table_t	*page_dir;
} pcb_t;



extern list_t* process_list;
extern pcb_t* current_process;
extern register_t saved_context;



#endif