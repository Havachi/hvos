#ifndef HVOS_PROCESS_H
#define HVOS_PROCESS_H

#include "kernel/vfs.h"
#include <stdint.h>
#include <sys/types.h>

#ifndef MAX_FILES_PER_PROCESS
#define MAX_FILES_PER_PROCESS 32
#endif

struct thread_s;

typedef struct process_s {
	uint64_t pid;
	uint64_t cr3;
	uint64_t heap_end;
	struct process_s *parent;
	int exit_code;
	bool is_waiting;
	uint64_t wait_target_pid;
	file_t *file_table[MAX_FILES_PER_PROCESS];
	struct thread_s *primary;
} process_t;


process_t *new_process();
process_t *new_elf_process(uint64_t rip, uint64_t ustack, uint64_t kstack);
void free_process(process_t *p);

process_t *process_by_pid(uint64_t pid);
void push_new_process(process_t *p);

#endif