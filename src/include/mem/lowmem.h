#ifndef HVOS_MEM_LOWMEM_H
#define HVOS_MEM_LOWMEM_H
#include "kernel/boot.h"
#define HHDM_OFFSET		hhdm_request.response->offset
#define IDT_BASE		(0x00001000 + HHDM_OFFSET)
#define IDT_BASE_PHYS	0x00001000

#define USER_SPACE_PHYS		0x0
#define USER_SPACE_END		0x00007FFFFFFFFFFF


#endif