#include "kernel/except.h"

static const char *s_exceptionDesc[20] =
{
    [0] = "Divide Error",
    [1] = "Debug",
    [2] = "Nonmaskable Interrupt",
    [3] = "Breakpoint",
    [4] = "Overflow",
    [5] = "Bound Range Exceeded",
    [6] = "Invalid Opcode",
    [7] = "Device Not Available",
    [8] = "Double Fault",
    [9] = "Coprocessor Segment Overrun",
    [10] = "Invalid TSS",
    [11] = "Segment Not Present",
    [12] = "Stack-Segment Fault",
    [13] = "General Protection",
    [14] = "Page Fault",
    [16] = "Floating Point Error",
    [17] = "Alignment Check",
    [18] = "Machine Check",
    [19] = "SIMD Exception"
};

void exception_dump(register_t regs)
{
    const char *desc = "Unknown";
    if (regs.int_num < 20)
    {
        desc = s_exceptionDesc[regs.int_num];
    }

    kprintf("Exception: %s\n", desc);
    kprintf("  rax=%016lx\n", regs.rax);
    kprintf("  rbx=%016lx\n", regs.rbx);
    kprintf("  rcx=%016lx\n", regs.rcx);
    kprintf("  rdx=%016lx\n", regs.rdx);
    kprintf("  rsi=%016lx\n", regs.rsi);
    kprintf("  rdi=%016lx\n", regs.rdi);
    kprintf("  rip=%016lx\n", regs.rip);
    kprintf("  rsp=%016lx\n", regs.rsp);
    kprintf("  cs= %02x\n", regs.cs);
    kprintf("  ss= %02x\n", regs.ss);

    for (;;) {}
}