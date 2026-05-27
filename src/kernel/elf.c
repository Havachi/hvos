#include "kernel/elf.h"
#include "kernel/scheduler/mt.h"
#include "kernel/scheduler/task.h"
#include "kernel/scheduler/task_state.h"
#include "kernel/vfs.h"
#include "mem/mem.h"
#include "klibc/printf.h"
#include <stdint.h>

uint64_t load_elf_binary(uint8_t *elf_buffer, uint64_t *out_pml4_phys) {
	elf64_header_t *header = (elf64_header_t *)elf_buffer;

	if (*(uint32_t *)header->e_ident != ELF_MAGIC) {
		kprintf("[ELF] Invalid magic\n");
		kfree(elf_buffer);
		return 0;
	}

	// allocate new pml4, get both physical and virtual addresses
	uint64_t pml4_phys = (uint64_t)pmm_alloc();
	pml4_table_t *pml4_virt = (pml4_table_t *)PHYS_TO_VIRT(pml4_phys);
	kmemset(pml4_virt, 0, PAGE_SIZE);
	kmemcpy(pml4_virt, kernel_pml4, PAGE_SIZE);  // inherit kernel mappings

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
				kprintf("[ELF] Out of memory\n");
				return 0;
			}
			map_page(pml4_virt, vaddr_page + off, phys_frame,
						PTE_PRESENT | PTE_WRITABLE | PTE_USER);

			// byte range this page covers in the segment
			uint64_t seg_off_start = (vaddr_page + off) - vaddr_start + page_offset;
			uint64_t bytes_copied = 0;

			if (seg_off_start < ph->p_filesz) {
				bytes_copied = ph->p_filesz - seg_off_start;
				if (bytes_copied > PAGE_SIZE) bytes_copied = PAGE_SIZE;
				kmemcpy((void *)PHYS_TO_VIRT(phys_frame),
						elf_buffer + ph->p_offset + seg_off_start,
						bytes_copied);
			}
			// zero remainder (handles .bss)
			if (bytes_copied < PAGE_SIZE)
				kmemset((void *)(PHYS_TO_VIRT(phys_frame) + bytes_copied),
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

	task_t *new_task = (task_t *)PHYS_TO_VIRT((uint64_t)pmm_alloc());
	kmemset(new_task, 0, sizeof(task_t));

	uint64_t k_stack_phys = (uint64_t)pmm_alloc();
	uint64_t *stack_top = (uint64_t *)PHYS_TO_VIRT(k_stack_phys + PAGE_SIZE);
	stack_top -= 15;
	stack_top--;
	*stack_top = entry_point;

	new_task->kernel_stack_base = stack_top;
	new_task->cr3   = pml4_phys;   // physical address for CR3
	new_task->vruntime = 0;
	new_task->state = STATE_READY;
	new_task->next  = NULL;
	return new_task;
}

int elf_load_and_run(const char *path) {
	vfs_node_t *file_node = vfs_finddir(vfs_root, path);
	if (!file_node) {
		kprintf("[ELF] %s: No such file or directory\n", path);
		return -1;
	}

	uint8_t *elf_buffer = (uint8_t *)kmalloc(file_node->size);
	vfs_read(file_node, 0, file_node->size, elf_buffer);

	task_t *new_task = create_elf_task(elf_buffer);
	if (!new_task) return -1;

	push_new_task(new_task);
	return new_task->pid;
}