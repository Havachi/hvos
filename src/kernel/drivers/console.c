#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include <drivers/console.h>
#include <stdio.h>
#include <string.h>

extern void schedule(void);
extern char keyboard_get_char(void);

long console_read(file_t *__file, char *buf, size_t __size, uint64_t *__offset) {
	(void)__file; (void)__offset;
	(void)__size;
	void *console_channel = (void *)console_read;
	while (1) {
		char c = keyboard_get_char();
		if (c == 0) {
			cpu_task_list_t *cpu = get_cpu_task_list();
			thread_t *t = cpu->current_thread;
			t->block_channel = console_channel;
			t->state = STATE_WAITING;
			schedule();
			continue;
		}
		*buf = c;
		return 1;
	}
}

long console_write(file_t *file, const char *buf, size_t count, uint64_t *offset) {
	(void)file;(void)offset;
	size_t i = 0;
	for (i = 0; i < count; i++) {
		put_char(buf[i]);
	}
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