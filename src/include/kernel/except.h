#ifndef HVOS_EXCEPT_H
#define HVOS_EXCEPT_H

#include "kernel/reg.h"
#include "klibc/printf.h"
#include <stdint.h>

typedef struct {
	union{
		uint32_t
			external:1,
			tbl: 2,
			index: 13;
		uint32_t _raw;
	};
} gf_error_code_t;


typedef struct {
	union {
		uint32_t p:1,w:1,u:1,r:1,i:1,pk:1,ss:1,reserved:8,sgx:1,reserved2:15;
		uint32_t _raw;
	};
} pagefault_error_code_t;

void exception_dump(register_t regs);

#endif