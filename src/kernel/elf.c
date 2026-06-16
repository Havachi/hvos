#include "kernel/elf.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/vfs.h"

#include "mem/mem.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

uint64_t load_elf_binary(uint8_t *elf_buffer, uint64_t *out_pml4_phys) {
	elf64_header_t *header = (elf64_header_t *)elf_buffer;

	if (*(uint32_t *)header->e_ident != ELF_MAGIC) {
		printf("[ELF] Invalid magic\n");
		kfree(elf_buffer);
		return 0;
	}

	// allocate new pml4, get both physical and virtual addresses
	uint64_t pml4_phys = (uint64_t)pmm_alloc();
	pml4_table_t *pml4_virt = (pml4_table_t *)PHYS_TO_VIRT(pml4_phys);
	memset(pml4_virt, 0, PAGE_SIZE);
	memcpy(pml4_virt, kernel_pml4, PAGE_SIZE);  // inherit kernel mappings

	elf64_pheader_t *pheaders = (elf64_pheader_t *)(elf_buffer + header->e_phoff);

	for (uint16_t i = 0; i < header->e_phnum; i++) {
		elf64_pheader_t *ph = &pheaders[i];
		if (ph->p_type != PT_LOAD) continue;

		uint64_t vaddr_start  = ph->p_vaddr;
		uint64_t page_offset  = vaddr_start % PAGE_SIZE;
		uint64_t vaddr_page   = vaddr_start - page_offset;
		uint64_t total_size   = ph->p_memsz + page_offset;

		for (uint64_t off = 0; off < total_size; off += PAGE_SIZE) {
			uint64_t phys_frame = (uint64_t)pmm_alloc();
			if (!phys_frame) {
				printf("[ELF] Out of memory\n");
				return 0;
			}
			map_page(pml4_virt, vaddr_page + off, phys_frame,
						PTE_PRESENT | PTE_WRITABLE | PTE_USER);

			// byte range this page covers in the segment
			uint64_t dest_page_offset = (off == 0) ? page_offset : 0;
			uint64_t file_read_offset = 
				(off == 0) ? 0 : (off - page_offset);
			uint64_t bytes_copied = 0;

			if (file_read_offset < ph->p_filesz) {
				uint64_t byte_to_copy = ph->p_filesz - file_read_offset;
				if (byte_to_copy > (PAGE_SIZE - dest_page_offset)) {
					byte_to_copy = PAGE_SIZE - dest_page_offset;
				}

				memcpy((void *)PHYS_TO_VIRT(phys_frame) + dest_page_offset,
						elf_buffer + ph->p_offset + file_read_offset,
						byte_to_copy);
				bytes_copied = byte_to_copy + dest_page_offset;
			} else {
				bytes_copied = dest_page_offset;
			}
			// zero remainder (handles .bss)
			if (bytes_copied < PAGE_SIZE)
				memset((void *)(PHYS_TO_VIRT(phys_frame) + bytes_copied),
						0, PAGE_SIZE - bytes_copied);
		}
	}

	*out_pml4_phys = pml4_phys;
	return header->e_entry;
}

task_t *create_elf_task(uint8_t *elf_buffer) {

	uint64_t pml4_phys = 0;
	uint64_t entry_point = load_elf_binary(elf_buffer, &pml4_phys);
	if (entry_point == 0) return NULL;
	
	pml4_table_t *pml4_virt = (pml4_table_t *)PHYS_TO_VIRT(pml4_phys);

	int num_stack_pages = 6;
	for (int i = 0; i < num_stack_pages; i++) {
		uint64_t user_stack_virt = USR_STACK_BASE - (i * PAGE_SIZE);
		uint64_t user_stack_phys = (uint64_t)pmm_alloc();
		if (!user_stack_phys){
			return NULL;
		}
		map_page(pml4_virt, user_stack_virt, user_stack_phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
	}

	uint64_t k_stack_phys = (uint64_t)pmm_alloc();
	uint64_t kernel_stack_top = (uint64_t)PHYS_TO_VIRT(k_stack_phys) + PAGE_SIZE;

	task_t *new_task = new_user_task(entry_point, USR_STACK_BASE, kernel_stack_top);
	new_task->cr3   = pml4_phys;
	new_task->vruntime = 0;
	new_task->state = STATE_READY;
	new_task->next  = NULL;
	new_task->heap_end = USR_HEAP_BASE;
	return new_task;
}

int elf_load_and_run(const char *path) {
	file_t *file = vfs_open(path, 0);
	if (!file) {
		printf("[ELF] %s: No such file or directory\n", path);
		return -1;
	}

	size_t file_size = file->f_dentry->d_inode->i_size;
	if (file_size == 0) {
		kfree(file);
		return -1;
	}

	uint8_t *elf_buffer = (uint8_t *)kmalloc(file_size);
	if (!elf_buffer) {
		kfree(file);
		return -1;
	}

	ssize_t byte_read = vfs_read(file, (char *)elf_buffer, file_size);

	if (byte_read < 0) {
		kfree(elf_buffer);
		kfree(file);
		return -1;
	}

	kfree(file);
	file = NULL;
	task_t *new_task = create_elf_task(elf_buffer);
	if (!new_task) return -1;
	new_task->parent = get_current_task();
	push_new_task(new_task);
	return new_task->pid;
}