#include "kernel/elf.h"
#include "kernel/vfs.h"
#include "mem/mem.h"
#include "kernel/mt.h"
#include "klibc/printf.h"

extern volatile pt_entry *current_pml4;

int elf_load_and_run(const char* path){
	vfs_node_t *file_node = vfs_finddir(vfs_root, path);
	if (!file_node) {
		kprintf("[ELF] Error loading ELF file: %s: No such file or directory\n", path);
		return -1;
	}

	uint8_t *elf_buffer = (uint8_t *)kmalloc(file_node->size);
	vfs_read(file_node, 0, file_node->size, elf_buffer);

	elf64_header_t *header = (elf64_header_t *)elf_buffer;

	if (*(uint32_t*) header->e_ident != ELF_MAGIC) {
		kprintf("[ELF] Error loading ELF file: %s: Invalid magic", path);
		kfree(elf_buffer);
		return -1;
	}

	uint64_t process_cr3 = vmm_create_address_space();
	pt_entry *process_pml4 = (pt_entry *)(process_cr3 + hhdm_offset);

	elf64_pheader_t *pheader = (elf64_pheader_t *)(elf_buffer + header->e_phoff);
	for (int i = 0; i < header->e_phnum; i++) {
		if (pheader[i].p_type == PT_LOAD) {
			uint64_t start_vaddr = pheader[i].p_vaddr;
			uint64_t end_vaddr = start_vaddr + pheader[i].p_memsz;

			uint64_t aligned_start = start_vaddr & ~(PAGE_SIZE - 1);
			uint64_t aligned_end = (end_vaddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

			for (uint64_t v = aligned_start; v < aligned_end; v += PAGE_SIZE) {
				void *phys_frame = pmm_alloc();
				vmm_map(process_pml4, v, (uint64_t) phys_frame, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
				uint64_t page_offset_in_segment = 0;
				if (v > start_vaddr) {
					page_offset_in_segment = v - start_vaddr;
				}

				int64_t bytes_to_copy = pheader[i].p_filesz - page_offset_in_segment;
				if (bytes_to_copy > (int64_t) PAGE_SIZE){
					bytes_to_copy = PAGE_SIZE;
				}
				uint8_t *hhdm_dest = (uint8_t *)((uint64_t)phys_frame + hhdm_offset);
				if (bytes_to_copy > 0) {
					uint8_t *source_ptr = elf_buffer + pheader[i].p_offset + page_offset_in_segment;
					kmemcpy(hhdm_dest, source_ptr, bytes_to_copy);
					if (bytes_to_copy < (int64_t) PAGE_SIZE) {
						kmemset(hhdm_dest + bytes_to_copy, 0, PAGE_SIZE-bytes_to_copy);
					}
				} else {
					kmemset(hhdm_dest, 0, PAGE_SIZE);
				}
			}
		}
	}
	
	extern task_t *create_user_task(void (*entry_point)(void), pt_entry *process_pml4, char *name);
	extern task_t tasks[];
	extern int nb_tasks;

	kprintf("Loading %s, entry at %016lx\n", path, header->e_entry);


	task_t* new_process = create_user_task((void(*)(void))header->e_entry, process_pml4, (char *)path);
	new_process->cr3 = process_cr3;
	new_process->state = TASK_STATE_RUNNING;

	kfree(elf_buffer);
	return 0;
}