#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task_state.h"
#include <drivers/console.h>

extern char keyboard_get_char(void);

static long console_read(file_t *file, char *buf, size_t size, uint64_t *offset) {
	(void)file; (void)offset;

	uint32_t bytes_read = 0;
	uint32_t written = 0;
	while (bytes_read < size) {
		
		char c = keyboard_get_char();
		if (c == 0) {
			task_t *current_task = get_current_task();
			current_task->state = STATE_WAITING;
			continue;
		} else {
			*buf++ = c;
			return 1;
			if (c == '\b') {
				if (written == 0) {
					continue;
				} else {
					written--;
				}
			}
			if (c == '\n') break;
		}
	}
	return bytes_read;
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
	f->f_lock = (spinlock_t) {.locked = 0};
	f->f_dentry = NULL;
	f->f_pos = 0;
	f->f_ops = &console_ops;
	f->f_flags = flags;
	return f;
}