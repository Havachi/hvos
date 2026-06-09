#include "kernel/except.h"
#include "kernel/scheduler/mt.h"
#include "klibc/printf.h"
#include <stdint.h>

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

void exception_dump(registers_t regs)
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

void page_fault_handler_c(fault_frame_t *f, uint64_t cr2) {
    kprintf("\n=== PAGE FAULT ===\n");
    kprintf("Faulting address (CR2):\t%016lx\n", cr2);
    kprintf("Error code:            \t%08lx\n", f->error_code);

    kprintf("Cause: %s %s in %s mode%s%s\n",
        (f->error_code & PF_PRESENT)  ? "protection violation" : "not present",
        (f->error_code & PF_WRITE)    ? "on write"             : "on read",
        (f->error_code & PF_USER)     ? "user"                 : "kernel",
        (f->error_code & PF_RESERVED) ? " [reserved bit set]"  : "",
        (f->error_code & PF_FETCH)    ? " [instruction fetch]" : ""
    );

    kprintf("\nRegisters:\n");
    kprintf("  rip\t%016lx\tcs\t%04lx\trflags\t%016lx\n",
            f->rip, f->cs, f->rflags);
    kprintf("  rsp\t%016lx\tss\t%04lx\n",
            f->rsp, f->ss);
    kprintf("  rax\t%016lx\trbx\t%016lx\trcx\t%016lx\n",
            f->rax, f->rbx, f->rcx);
    kprintf("  rdx\t%016lx\trsi\t%016lx\trdi\t%016lx\n",
            f->rdx, f->rsi, f->rdi);
    kprintf("  rbp\t%016lx\tr8\t%016lx\tr9\t%016lx\n",
            f->rbp, f->r8,  f->r9);
    kprintf("  r10\t%016lx\tr11\t%016lx\tr12\t%016lx\n",
            f->r10, f->r11, f->r12);
    kprintf("  r13\t%016lx\tr14\t%016lx\tr15\t%016lx\n",
            f->r13, f->r14, f->r15);

    kprintf("\nSystem halted.\n");
    for (;;) asm volatile("hlt");
}

void gpf_execption_handler_c(uint64_t rip, uint64_t err) {
    gf_error_code_t err_code = {0};
    err_code._raw = err;
    kprintf("GPF!!!!\nRIP: %lx\nERR:%lx\n", rip, err);
    kprintf("Error from: ");
    if (err_code.external == 1) {
        kprintf("External\n");
    }
    if (err_code.tbl == 0) {
        kprintf("GDT\n");
    } else if (err_code.tbl == 1 || err_code.tbl == 3) {
        kprintf("IDT\n");
    } else if (err_code.tbl == 2) {
        kprintf("LDT\n");
    }
    kprintf("index: %lx\n", err_code.index);
    kprintf("Halting cpu !\n");
    for(;;){
        asm volatile("hlt");
    }
}