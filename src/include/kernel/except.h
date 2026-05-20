#ifndef HVOS_EXCEPT_H
#define HVOS_EXCEPT_H

#include "kernel/reg.h"
#include "klibc/printf.h"

void exception_dump(register_t regs);

#endif