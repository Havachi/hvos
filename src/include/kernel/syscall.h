#ifndef HVOS_SYSCALL_H
#define HVOS_SYSCALL_H

#include <stdint.h>

#define MAX_FD          16
#define MSR_STAR		0xC0000081
#define MSR_LSTAR		0xC0000082
#define MSR_SFMASK		0xC0000084
#define MSR_EFER        0xC0000080
typedef struct {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
	uint64_t rip, cs, rflags, rsp, ss;
} syscall_frame_t;



extern void wrmsr(uint32_t, uint64_t);
extern uint64_t rdmsr(uint32_t msr);
extern void syscall_entry_asm(void);

void init_syscall(void);
void sys_print(const char* str);
void syscall_handler(syscall_frame_t* frame);

#endif
