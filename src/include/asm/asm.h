#ifndef HVOS_ASM_ASM_H
#define HVOS_ASM_ASM_H

#include "hvos/compiler_attributes.h"


#ifndef __ASSEMBLER__
static __always_inline __pure void *rip_rel_ptr(void *p) {
    __asm__("leaq (%1), %0" : "=r" (p) : "p" (p));
	return p;
}

static __always_inline void cli() {
	__asm__ __volatile__("cli": : :"memory");
}

static __always_inline void sti() {
	__asm__ __volatile__("sti": : :"memory");
}

#endif

#endif
