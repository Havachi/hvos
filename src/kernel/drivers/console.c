#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include <drivers/console.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern char keyboard_get_char(void);

safe_lock_t console_lock = {.locked = 0};
static thread_t *console_read_owner = NULL;
long console_read(file_t *__file, char *buf, size_t __size, uint64_t *__offset) {
	(void)__file; (void)__offset;
	(void)__size;
	cpu_task_list_t *cpu = get_cpu_task_list();
	thread_t * t = get_current_thread();

	void *console_channel = (void *)console_read;

	while (console_read_owner != NULL && console_read_owner != t) {
		t->block_channel = console_channel;
		t->state = STATE_WAITING;
		kernel_yield();
	}

	console_read_owner = t;

	while (1) {
		char c = keyboard_get_char();
		if (c == 0) {
			t->block_channel = console_channel;
			t->state = STATE_WAITING;
			kernel_yield();
			continue;
		}
		*buf = c;
		console_read_owner = NULL;
		for (int i = 0; i < cpu->thread_list->count; i++) {
			thread_t *other = (thread_t *)list_get_at(cpu->thread_list, i); 
			if (other->state == STATE_WAITING && other->block_channel == console_channel) {
				other->state = STATE_READY;
				other->block_channel = NULL;
			}
		}
		return 1;
	}
}

long console_write(file_t *file, const char *buf, size_t count, uint64_t *offset) {
	(void)file;(void)offset;
	uint64_t flags = safe_lock(&console_lock);
	size_t i = 0;
	for (i = 0; i < count; i++) {
		put_char(buf[i]);
	}
	safe_unlock(&console_lock, flags);
	return i;
}

static file_ops_t console_ops = {
	.read = console_read,
	.write = console_write
};

file_t *create_kernel_console_file(int flags) {
	file_t *f = kmalloc(sizeof(file_t ));
	if (f == NULL) {
		return NULL;
	}
	memset(f, 0x0, sizeof(file_t));
	f->f_lock = (spinlock_t) {.locked = 0};
	f->f_dentry = NULL;
	f->f_pos = 0;
	f->f_ops = &console_ops;
	f->f_flags = flags;
	return f;
}