#ifndef HVOS_ASM_ASM_H
#define HVOS_ASM_ASM_H

#include "hvos/compiler_attributes.h"


#ifndef __ASSEMBLER__
static __always_inline __pure void *rip_rel_ptr(void *p) {
    asm("leaq (%1), %0" : "=r" (p) : "p" (p));
	return p;
}
#endif

#endif
