#include "kernel/syscall.h"
#include "asm/asm.h"
#include "kernel/gdt.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/smp.h"
#include "kernel/sync.h"
#include "kernel/time.h"
#include "kernel/tss.h"
#include "kernel/vfs.h"
#include "kernel/syscall_id.h"
#include "kernel/elf.h"
#include <errno-list.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>



extern task_t tasks[];

extern char keyboard_get_char(void);
extern uint64_t kernel_stack_pointer;

void init_syscall_gs() {
	cpu_data_t *cpu = get_current_cpu_data();
	uint64_t addr = (uint64_t)cpu;
	uint32_t low = (uint32_t)(addr & 0xFFFFFFFF);
	uint32_t high = (uint32_t)(addr >> 32);

	__asm__ volatile("wrmsr" : : "c"(MSR_KERNEL_GS_BASE), "a"(low), "d"(high));
}

void init_syscall(void) {
    uint64_t efer = rdmsr(MSR_EFER);
    efer |= (1ULL << 0);
    wrmsr(MSR_EFER, efer);
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry_asm);
    uint64_t star_val = (((uint64_t)(__USER_CS - 16) & 0xFFF8) << 48) | (((uint64_t)__KERNEL_CS & 0xFFF8) << 32);
    wrmsr(MSR_STAR, star_val);
    wrmsr(MSR_SFMASK, 0xFFFFFFFFFFFFFFFD); 
	init_syscall_gs();
}


int sys_open(const char *path, int flags) {
	if (root_dentry == NULL || path == NULL) return -1;
	if (path[0] == '/') path++;

	int fd = -1;
	for (int i = 0; i < MAX_FILES_PER_PROCESS; i++) {
		if (get_current_process()->file_table[i] == NULL) {
			fd = i;
			break;
		}
	}
	if (fd == -1){
		return -EMFILE;
	}
	dentry_t *dentry = vfs_lookup(path);
	if (!dentry || !dentry->d_inode) {
		fprintf(stderr, "[SYSCALL] sys_open: %s: No such file or directory\n", path);
		return -ENOENT;
	}

	file_t *file = kmalloc(sizeof(file_t));
	if (!file) {
		return -ENOMEM;
	}
	file->f_lock.locked = 0;
	file->f_dentry = dentry;
	file->f_pos = 0;
	file->f_ops = dentry->d_inode->i_fop;
	file->f_flags = flags;
	get_current_process()->file_table[fd] = file;
	return fd;
}

long sys_read(unsigned int fd, char *buffer, size_t size) {
	if (buffer == NULL || size == 0) return -1;

	if ((get_current_process()->file_table[fd]->f_flags & 0x3) == O_WRONLY){
		return -EBADF;
	}
	if (fd < 0 || fd >= MAX_FD || get_current_process()->file_table[fd] == NULL ) return -1;
	return (long)vfs_read(get_current_process()->file_table[fd], (char *)buffer,  size);
}



long sys_write(unsigned int fd, const char *buffer, size_t size) {
	/*if (!is_valid_user_address(buffer, size)) {
		return -EFAULT;
	}*/
	if (fd < 0 || fd >= MAX_FILES_PER_PROCESS){
		return -EBADF;
	}

	file_t *file = get_current_process()->file_table[fd];
	if (!file || !file->f_ops || !file->f_ops->write || ((file->f_flags & 0x3) == O_RDONLY)) {
		return -EBADF;
	} 

	spin_lock(&file->f_lock);
	long written = file->f_ops->write(file, buffer, size, &file->f_pos);
	spin_unlock(&file->f_lock);
	return written;
}

void sys_print(const char *str) {
	printf("%s", str);
}

void sys_exit(int code) {
	cpu_task_list_t *cpu = get_cpu_task_list();
	process_t *current_proc = get_current_process();
	thread_t *current_thread = cpu->current_thread;
	current_proc->exit_code = code;
	current_thread->state = STATE_DEAD;

	process_t *parent = current_proc->parent;
	if (parent && parent->is_waiting && parent->wait_target_pid == current_proc->pid) {
		parent->is_waiting = false;
		if (parent->primary)
			parent->primary->state = STATE_READY;
	}
	schedule();
	while(1);
}

int sys_exec(const char *path) {
	if (path == NULL) return -1;
	return execute_elf(path);
}

int sys_time(uint64_t *ptr) {
	datetime_t *dt = now();
	timestamp_t ts = dttots(dt);
	*ptr = ts;
	return 0;
}

int sys_waitpid(uint64_t pid) {
	cpu_task_list_t *cpu = get_cpu_task_list();
	process_t *current_proc = get_current_process();
	thread_t *current_thread = cpu->current_thread;
	process_t *child = process_by_pid(pid);
	if (!child)
		return -1;
	if (child->primary && child->primary->state == STATE_DEAD) {
		int code = child->exit_code;
		return code;
	}

	current_proc->is_waiting = true;
	current_proc->wait_target_pid = pid;
	current_thread->state = STATE_WAITING;
	schedule();
	return child->exit_code;
}

uint64_t sys_alloc_pages(uint32_t pages) {
	if (pages == 0) return 0;

	process_t *ct = get_current_process();
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

int sys_waitid(int which, pid_t pid, void *_infop, int _opt, void *_ru) {
	return sys_waitpid(pid);
}

int sys_fork(void) {
	return -1;
}

void syscall_handler(pt_regs_t *frame) {
	uint64_t syscall_number = frame->orig_ax;

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
			frame->rax = sys_open((const char *)frame->rdi, (int)frame->rsi);
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

		case SC_WAITID:
			frame->rax = sys_waitid(frame->rdi, frame->rsi, (void *)frame->rdx, frame->r10, (void *)frame->r8);
			break;
		default:
			printf("Unknown syscall: %d\n", syscall_number);
			break;
	}
}