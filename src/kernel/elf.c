#include "kernel/elf.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/vfs.h"
#include "mem/mem.h"
#include "klibc/printf.h"
#include <stdint.h>

extern volatile pt_entry *current_pml4;


uint64_t load_elf_binary(uint8_t *elf_buffer, uint64_t **task_pml4) {


	elf64_header_t *header = (elf64_header_t *)elf_buffer;

	if (*(uint32_t*) header->e_ident != ELF_MAGIC) {
		kprintf("[ELF] Error loading ELF file: Invalid magic");
		kfree(elf_buffer);
		return 0;
	}

	pml4_table_t *pml4 = create_new_pml4();

	elf64_pheader_t *pheader = (elf64_pheader_t *)(elf_buffer + header->e_phoff);

	for (uint16_t i = 0; i < header->e_phnum; i++) {
		if (pheader[i].p_type == PT_LOAD) {
			uint64_t mem_size = pheader[i].p_memsz;
			uint64_t vaddr_start = pheader[i].p_vaddr;
			uint64_t page_offset = vaddr_start % PAGE_SIZE;
			uint64_t vaddr_page = vaddr_start - page_offset;
			uint64_t total_size = mem_size + page_offset;

			for (uint64_t offset = 0; offset < total_size; offset += PAGE_SIZE) {
				void *phys_frame = pmm_alloc();
				map_page(pml4, (vaddr_page + offset), 
				(uint64_t)phys_frame, PTE_PRESENT|PTE_WRITABLE|PTE_USER);
			}
			uint64_t old_cr3;
			kprintf("it is stuck here ?\n");
			asm volatile("mov %%cr3, %0" : "=r"(old_cr3));
            asm volatile("mov %0, %%cr3" :: "r"(create_new_pml4));
			kprintf("nope\n");
			kmemcpy((void*)vaddr_start, elf_buffer + pheader[i].p_offset, pheader[i].p_filesz);
			if (pheader[i].p_memsz > pheader[i].p_filesz) {
            	kmemset((void*)(vaddr_start + pheader[i].p_filesz), 0, pheader[i].p_memsz - pheader[i].p_filesz);
            }
			asm volatile("mov %0, %%cr3" :: "r"(old_cr3));
		}
	}
	*task_pml4 = (uint64_t*)&pml4;
	return header->e_entry;
}

task_t *create_elf_task(uint8_t *elf_buffer) {
	uint64_t *task_pml4 = NULL;
	uint64_t entry_point = load_elf_binary(elf_buffer, &task_pml4);
	if (entry_point == 0) return  NULL;

	task_t *new_task = (task_t *)pmm_alloc();
	void *k_stack_raw = pmm_alloc();
	uint64_t *stack_top = (uint64_t *)((uint64_t)k_stack_raw + PAGE_SIZE);
	stack_top -=15;

	stack_top--; *stack_top = entry_point;
	new_task->kernel_stack_base = stack_top;
	new_task->cr3 = (uint64_t)task_pml4;
	new_task->vruntime = 0;
	new_task->state = STATE_READY;
	new_task->next = NULL;
	return new_task;
}


int elf_load_and_run(const char* path){
	vfs_node_t *file_node = vfs_finddir(vfs_root, path);
	if (!file_node) {
		kprintf("[ELF] Error loading ELF file: %s: No such file or directory\n", path);
		return -1;
	}

	uint8_t *elf_buffer = (uint8_t *)kmalloc(file_node->size);
	vfs_read(file_node, 0, file_node->size, elf_buffer);

	task_t *new_task = create_elf_task(elf_buffer);
	push_new_task(new_task);
	return new_task->pid;
}
