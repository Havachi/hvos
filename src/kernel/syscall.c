#include "kernel/syscall.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/sync.h"
#include "kernel/time.h"
#include "kernel/video.h"
#include "kernel/vfs.h"
#include "kernel/syscall_id.h"
#include "kernel/elf.h"
#include <stdint.h>
#include <stdio.h>


static file_t* open_files[MAX_FD] = {NULL};
extern task_t tasks[];

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
	if (root_dentry == NULL || path == NULL) return -1;
	if (path[0] == '/') path++;
	dentry_t *dentry = vfs_lookup(path);
	if (dentry == NULL) {
		printf("[SYSCALL] sys_open: %s: No such file or directory\n", path);
		return -1;
	}
	for (int i = 0; i < MAX_FD; i++) {
		if (open_files[i] == NULL) {
			open_files[i] = vfs_open(path, 0);
			return i;
		}
	}
	return -1;
}

long sys_read(unsigned int fd, char *buffer, size_t size) {
	if (buffer == NULL || size == 0) return -1;
	if (fd == 0) {
		uint32_t bytes_read = 0;
		uint32_t written = 0;
		while (bytes_read < size) {

			char c = keyboard_get_char();
			if (c == 0) {
				task_t *current_task = get_current_task();
				current_task->state = STATE_WAITING;
				continue;
			} else {
				*buffer++ = c;
				return 1;
				if (c == '\b') {
					if (written == 0) {
						continue;
					} else {
						written--;
					}
				}
				printf("%c", c); 
				if (c == '\n') break;
			}

		}
		return bytes_read;
	}

	if (fd < 0 || fd >= MAX_FD || open_files[fd] == NULL) return -1;
	return vfs_read(open_files[fd], (char *)buffer,  size);
}



long sys_write(unsigned int fd, const char *buffer, size_t size) {
	if (buffer == NULL || size == 0) return -1;
	if (fd == 1) {
		for (uint32_t i = 0; i < size; i++) {
			if (buffer[i] == 0x02){
				clear_screen();
			} else {
				printf("%c", buffer[i]);
			}
		}
		return size;
	}
	if (fd == 2) {
		for (uint32_t i = 0; i < size; i++) {
			fprintf(stderr, "%c", buffer[i]);
		}
		return size;
	}
	return -1;
}

void sys_print(const char *str) {
	printf("%s", str);
}

void sys_exit(int code) {
	task_t *ct = get_current_task();
	//printf("\n[KERNEL] process %d exited with code: %d\n", ct->pid, code);
	ct->state = STATE_DEAD;
	__asm__ volatile("int $0x30");
}

int sys_exec(const char *path) {
	if (path == NULL) return -1;
	return elf_load_and_run(path);
}

int sys_time(uint64_t *ptr) {
	datetime_t *dt = now();
	timestamp_t ts = dttots(dt);
	*ptr = ts;
	return 0;
}

uint64_t sys_alloc_pages(uint32_t pages) {
	if (pages == 0) return 0;

	task_t *ct = get_current_task();
	pml4_table_t *pm = (pml4_table_t *) PHYS_TO_VIRT(ct->cr3);

	uint64_t virt_start = ct->heap_end;

	for (uint64_t i = 0; i < pages; i++) {
		void *phy_page = pmm_alloc();
		if (phy_page == NULL) {
			return 0;
		}
		uint64_t curr_virt = virt_start + (i * PAGE_SIZE);
		uint64_t curr_phys = (uint64_t) phy_page;
		map_page(pm, curr_virt, curr_phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
	}

	ct->heap_end += (pages * PAGE_SIZE);
	return virt_start;
}

int sys_free_pages(void *ptr, uint32_t pages) {
	if (!ptr)
		return -1;
	kfree(ptr);
	return 0;
}

void syscall_handler(stack_frame_t *frame) {
	uint64_t syscall_number = frame->rax;
	switch (syscall_number) {
		case SC_UPRINT:
			sys_print((const char *)frame->rdi);
			break;
		case SC_READ:
			frame->rax = sys_read((int)frame->rdi, (uint8_t *)frame->rsi, (uint32_t)frame->rdx);
			break;
		case SC_WRITE:
			frame->rax = sys_write((int)frame->rdi, (uint8_t *)frame->rsi, (uint32_t)frame->rdx);
			break;
		case SC_OPEN:
			frame->rax = sys_open((const char *)frame->rdi);
			break;
		case SC_MMAP:
			frame->rax = sys_alloc_pages(frame->rsi);
			break;
		case SC_MUNMAP:
			frame->rax = sys_free_pages((void *)frame->rdi, frame->rsi);
			break;
		case SC_EXIT:
			sys_exit((int)frame->rdi);
			break;
		case SC_YIELD:
			__asm__ volatile("int $0x20");
			break;
		case SC_EXECVE:
			frame->rax = sys_exec((const char *)frame->rdi);
			break;
		case SC_TIME:
			frame->rax = sys_time((uint64_t *)frame->rdi);
			break;
		default:
			printf("Unknown syscall: %d\n", syscall_number);
			break;
	}
}