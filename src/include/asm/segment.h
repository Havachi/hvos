#ifndef HVOS_SEGMENTS_H
#define HVOS_SEGMENTS_H
#include <stdint.h>


/* Entries used at boot */
#define GDT_ENTRY_BOOT_CS			2
#define GDT_ENTRY_BOOT_DS			3
#define GDT_ENTRY_BOOT_TSS			4
#define _BOOT_CS					(GDT_ENTRY_BOOT_CS*8)
#define _BOOT_DS					(GDT_ENTRY_BOOT_DS*8)
#define _BOOT_TSS					(GDT_ENTRY_BOOT_TSS*8)

/* x86-64 */
#define GDT_ENTRY_KERNEL_CS			1
#define GDT_ENTRY_KERNEL_DS			2
#define GDT_ENTRY_DEFAULT_USER_DS	3
#define GDT_ENTRY_DEFAULT_USER_CS	4
/* Needs two entries */
#define GDT_ENTRY_TSS				6
/* Needs two entries */
#define GDT_ENTRY_LDT				8
#define GDT_ENTRY_TLS_MIN			9
#define GDT_ENTRY_TLS_MAX			10
#define GDT_ENTRY_CPUNODE			11

#define __KERNEL_CS			(GDT_ENTRY_KERNEL_CS*8)
#define __KERNEL_DS			(GDT_ENTRY_KERNEL_DS*8)
#define __USER_CS			(GDT_ENTRY_DEFAULT_USER_CS*8 + 3)
#define __USER_DS			(GDT_ENTRY_DEFAULT_USER_DS*8 + 3)

/*
 * Number of entries in the GDT table:
 */
#define GDT_ENTRIES					12
#define GDT_SIZE					(GDT_ENTRIES*8)

#define IDT_ENTRIES					256
#define NUM_EXCEPTION_VECTORS		32

#define GDT_ENTRY_TLS_ENTRIES		3
#define TLS_SIZE					(GDT_ENTRY_TLS_ENTRIES* 8)

#endif /* HVOS_SEGMENTS_H */