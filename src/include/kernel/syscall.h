#ifndef HVOS_SYSCALL_H
#define HVOS_SYSCALL_H

#include "kernel/scheduler/mt.h"
#include <stdint.h>

#define MAX_FD          16
#define MSR_STAR		0xC0000081
#define MSR_LSTAR		0xC0000082
#define MSR_SFMASK		0xC0000084
#define MSR_EFER        0xC0000080




extern void wrmsr(uint32_t, uint64_t);
extern uint64_t rdmsr(uint32_t msr);
extern void syscall_entry_asm(void);

void init_syscall(void);
void sys_print(const char* str);
void syscall_handler(stack_frame_t* frame);

#endif
