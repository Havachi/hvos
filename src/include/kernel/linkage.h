#ifndef _HVOS_LINKAGE_H
#define _HVOS_LINKAGE_H

#ifndef ASM_NL
#define ASM_NL		 ;
#endif

#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif

#ifndef asmlinkage
#define asmlinkage CPP_ASMLINKAGE
#endif


#ifndef __ASSEMBLY__
#ifndef asmlinkage_protect
#define asmlinkage_protect(n, ret, args...) do { } while (0)
#endif
#endif

#ifndef __ALIGN
#define __ALIGN .align 4,0x90
#define __ALIGN_STR ".align 4,0x90"
#endif

#ifdef __ASSEMBLY__

#ifndef LINKER_SCRIPT
#define ALIGN _ALIGN
#define ALIGN_STR _ALIGN_STR

#ifndef ENTRY
#define ENTRY(name) \
	.globl name ASM_NL \
	ALIGN ASM_NL \
	name:
#endif
#endif

#ifndef WEAK
#define WEAK(name)	   \
	.weak name ASM_NL   \
	name:
#endif

#ifndef END
#define END(name) \
	.size name, .-name
#endif

#ifndef ENDPROC
#define ENDPROC(name) \
	.type name, @function ASM_NL \
	END(name)
#endif

#endif

#endif