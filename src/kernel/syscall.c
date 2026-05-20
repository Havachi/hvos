#include "kernel/syscall.h"

static vfs_node_t* open_files[MAX_FD] = {NULL};
extern task_t tasks[];
extern int current_task;

extern char keyboard_get_char(void);

void init_syscall(void) {
    uint64_t efer = rdmsr(MSR_EFER);
    efer |= (1ULL << 0);
    wrmsr(MSR_EFER, efer);

    // Your existing LSTAR, STAR, and SFMASK code continues here...
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry_asm);
    uint64_t star_val = ((uint64_t)0x1B << 48) | ((uint64_t)0x08 << 32);
    wrmsr(MSR_STAR, star_val);
    wrmsr(MSR_SFMASK, 0x200); 
}

int sys_open(const char *path) {
	if (vfs_root == NULL || path == NULL) return -1;
	if (path[0] == '/') path++;
	vfs_node_t *node = vfs_finddir(vfs_root, path);
	if (node == NULL) {
		kprintf("[SYSCALL] sys_open: %s: No such file or directory\n", path);
		return -1;
	}

	for (int i = 0; i < MAX_FD; i++) {
		if (open_files[i] == NULL) {
			open_files[i] = node;
			return i;
		}
	}

	return -1;
}

int sys_read(int fd, uint8_t *buffer, uint32_t size) {

	if (buffer == NULL || size == 0) return -1;
	
	if (fd == 0) {
		uint32_t bytes_read = 0;
		while (bytes_read < size) {
			char c = keyboard_get_char();
			if (c == 0) {
				tasks[current_task].state = TASK_STATE_BLOCKED_ON_KEYBOARD;
				yield();
				continue;
			}
			buffer[bytes_read++] = c;
			if (c == '\n') break;
		}
		return bytes_read;
	}

	if (fd < 0 || fd >= MAX_FD || open_files[fd] == NULL) return -1;
	return vfs_read(open_files[fd], 0, size, buffer);
}

int sys_write(int fd, uint8_t *buffer, uint32_t size) {
	if (buffer == NULL || size == 0) return -1;
	if (fd == 1) {
		for (uint32_t i = 0; i < size; i++) {
			put_char((char)buffer[i]);
		}
		return size;
	}
	return -1;
}

void sys_print(const char *str) {
	kprintf("%s", str);
}

void sys_exit(int code) {
	kprintf("\n[KERNEL] process %d exited with code: %d\n", current_task, code);
	tasks[current_task].state = TASK_STATE_DEAD;
	tasks[current_task].exit_code = code;
	asm volatile("int $0x20");
}

int sys_exec(const char *path) {
	if (path == NULL) return -1;
	return elf_load_and_run(path);
}

void syscall_handler(syscall_frame_t *frame) {
	uint64_t syscall_number = frame->rax;
	switch (syscall_number) {
		case SC_UPRINT:
			sys_print((const char *)frame->rdi);
			break;
		case SC_READ:
			frame->rax = sys_read((int)frame->rdi, (uint8_t *)frame->rsi, (uint32_t)frame->rdx);
			break;
		case SC_WRITE:
			frame->rax = sys_write((int)frame->rdi, (const uint8_t *)frame->rsi, (uint32_t)frame->rdx);
			break;
		case SC_OPEN:
			frame->rax = sys_open((const char *)frame->rdi);
			break;

		case SC_EXIT:
			sys_exit((int)frame->rdi);
			break;
		case SC_YIELD:
			asm volatile("int $0x20");
			break;
		case SC_EXECVE:
			frame->rax = sys_exec((const char *)frame->rdi);
			break;
		default:
			kprintf("Unknown syscall: %d\n", syscall_number);
			break;
	}
}