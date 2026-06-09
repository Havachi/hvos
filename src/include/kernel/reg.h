#ifndef HVOS_REG_H
#define HVOS_REG_H

#include <stdint.h>

typedef struct registers_s
{
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t int_num, errCode;
    uint64_t rip, cs, eflags, rsp, ss;
} registers_t;



#endif