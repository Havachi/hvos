#ifndef HVOS_MEM_PAGING_H
#define HVOS_MEM_PAGING_H

#include <stdint.h>
#include "hvos/compiler_attributes.h"
#define PAGE_SIZE 0x1000
#define PAGE_MASK (PAGE_SIZE - 1)

#define IS_ALIGNED(addr) ((((uint64_t)(addr)) % PAGE_SIZE) == 0)
#define PAGE_ALIGN(addr) ((((uint64_t)(addr)) + PAGE_MASK) & ~(PAGE_MASK))

#define ENTRIES_PER_TABLE 512

typedef struct {
	uint64_t
		present:1,		//Present
		rw:1,			//Read/Write
		us:1,			//User/Supervisor
		pwt:1,			//Write/Through
		pcd:1,			//Cache Disable
		accessed:1, 	//Accessed
		dirty:1,		//Dirty
		pat:1,			//Page Attribute Table | 0 for PML4/PT, 1 for huge page in PDPT/PD
		global:1,		//Global | Prevents TLB flush on CR3 reload
		avl_low:3,		//Available to OS
		address:40,		//Address
		avl_high:11, 	//Available/Reserved	
		xd:1			//Execute Disable	
		;
} __packed page_table_entry_t ;

typedef struct {
	page_table_entry_t entries[ENTRIES_PER_TABLE];
} __packed page_table_t; 

//Page Map Level 4
typedef page_table_t pml4_table_t;
//Page Directory Pointer Table
typedef page_table_t pdpt_table_t;
//Page Directory
typedef page_table_t pd_table_t;
//Page Table
typedef page_table_t pt_table_t;		

typedef struct {
	uint64_t
		offset: 12,
		pt_index: 9,
		pd_index: 9,
		pdpt_index: 9,
		pml4_index: 9,
		sign_extend: 16
	;
} __packed virtual_address_t;

#endif