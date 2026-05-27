#ifndef HVOS_EXCEPT_H
#define HVOS_EXCEPT_H

#include "kernel/reg.h"
#include "kernel/scheduler/mt.h"
#include <stdint.h>

#define PF_PRESENT   (1 << 0)
#define PF_WRITE     (1 << 1)
#define PF_USER      (1 << 2)
#define PF_RESERVED  (1 << 3)
#define PF_FETCH     (1 << 4)

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

typedef struct {
    // manually pushed (in push order, so reversed in memory)
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    // CPU pushed
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) fault_frame_t;

void exception_dump(register_t regs);
void page_fault_handler_c(fault_frame_t *f, uint64_t cr2);

#endif