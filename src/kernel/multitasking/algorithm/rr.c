/*
	Round robin scheduling
*/


#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/scheduler/thread.h"
#include <stdint.h>
#include <string.h>

static unsigned long next_id = 0;

thread_t *get_next_thread_rr() {
	cpu_task_list_t *cpu = get_cpu_task_list();
	list_t *thread_list = cpu->thread_list;

	if (thread_list->count == 0)
		return NULL;

	for (uint32_t checked = 0; checked < thread_list->count; checked++) {
		if (next_id >= thread_list->count) next_id = 0;
	
		thread_t *candidate = (thread_t *)(list_get_at(thread_list, next_id++));

		if (strcmp(candidate->name, "IDLE_THREAD") == 0) {
			continue;
		}
		
		if (candidate->state == STATE_READY) {
			return candidate;
		}
	}
	return NULL;
}