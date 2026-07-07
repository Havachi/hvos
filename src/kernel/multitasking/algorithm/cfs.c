
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/scheduler/thread.h"
#include <stdint.h>
#include <string.h>

static thread_t * get_lowest_runtime() {
	cpu_task_list_t *cpu = get_cpu_task_list();
	list_t *thread_list = cpu->thread_list;

	if (thread_list == NULL || thread_list->count == 0)
		return NULL;
	thread_t *next = NULL;
	for (uint64_t i = 0; i < thread_list->count; i++) {
		thread_t *t = list_get_at(thread_list, i);
		if (t->state == STATE_READY) {
			if (next == NULL) {
				next = t;
				continue;
			}
			if (t->vruntime < next->vruntime) {
				next = t;
				continue;
			}
		}
	}
	return next;
}

thread_t *get_next_thread_cfs() {
	thread_t *next_thread = get_lowest_runtime();
	return next_thread;
}