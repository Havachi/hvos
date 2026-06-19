#include "kernel/except.h"
#include "kernel/scheduler/mt.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

    printf("Exception: %s\n", desc);
    printf("  rax=%016lx\n", regs.rax);
    printf("  rbx=%016lx\n", regs.rbx);
    printf("  rcx=%016lx\n", regs.rcx);
    printf("  rdx=%016lx\n", regs.rdx);
    printf("  rsi=%016lx\n", regs.rsi);
    printf("  rdi=%016lx\n", regs.rdi);
    printf("  rip=%016lx\n", regs.rip);
    printf("  rsp=%016lx\n", regs.rsp);
    printf("  cs= %02x\n", regs.cs);
    printf("  ss= %02x\n", regs.ss);

    abort();
}

void unhandled_page_fault(fault_frame_t *f, uint64_t cr2) {
    printf("\n=== PAGE FAULT ===\n");
    printf("Faulting address (CR2):\t0x%lx\n", cr2);
    printf("Error code:            \t0x%lx\n", f->error_code);

    printf("Cause: %s %s in %s mode%s%s\n",
        (f->error_code & PF_PRESENT)  ? "protection violation" : "not present",
        (f->error_code & PF_WRITE)    ? "on write"             : "on read",
        (f->error_code & PF_USER)     ? "user"                 : "kernel",
        (f->error_code & PF_RESERVED) ? " [reserved bit set]"  : "",
        (f->error_code & PF_FETCH)    ? " [instruction fetch]" : ""
    );

    printf("\nRegisters:\n");
    printf("  rip\t%016lx\tcs\t%04lx\trflags\t%016lx\n",
            f->rip, f->cs, f->rflags);
    printf("  rsp\t%016lx\tss\t%04lx\n", 
            f->rsp, f->ss);
    printf("  rax\t%016lx\trbx\t%016lx\trcx\t%016lx\n",
            f->rax, f->rbx, f->rcx);
    printf("  rdx\t%016lx\trsi\t%016lx\trdi\t%016lx\n",
            f->rdx, f->rsi, f->rdi);
    printf("  rbp\t%016lx\tr8\t%016lx\tr9\t%016lx\n",
            f->rbp, f->r8,  f->r9);
    printf("  r10\t%016lx\tr11\t%016lx\tr12\t%016lx\n",
            f->r10, f->r11, f->r12);
    printf("  r13\t%016lx\tr14\t%016lx\tr15\t%016lx\n",
            f->r13, f->r14, f->r15);

    printf("\nSystem halted.\n");
    abort();
}

void page_fault_handler_c(fault_frame_t *f, uint64_t cr2) {
    bool is_user = (f->error_code & PF_USER) != 0;
    bool is_present = (f->error_code & PF_PRESENT) != 0;
    uint64_t stack_limit = USR_STACK_BASE - (16 * PAGE_SIZE);
    if (is_user && !is_present && cr2 < USR_STACK_BASE && cr2 >= stack_limit) {
        uint64_t fault_page_virt = cr2 & ~(PAGE_SIZE - 1);
        uint64_t new_page_phys = (uint64_t)pmm_alloc();
        if (new_page_phys != 0) {
            cpu_task_list_t *cpu = get_cpu_task_list();
            process_t *current_process = cpu->current_thread->process; 
            pml4_table_t *pml4_virt = (pml4_table_t *)PHYS_TO_VIRT(current_process->cr3);
            map_page(pml4_virt, fault_page_virt, new_page_phys, 
                    PTE_PRESENT | PTE_WRITABLE | PTE_USER);
            return;
        }
    }
    unhandled_page_fault(f,cr2);
}



void gpf_execption_handler_c(uint64_t rip, uint64_t err) {
    gf_error_code_t err_code = {0};
    err_code._raw = err;
    printf("\n==== General Fault Protection ====\n\n", rip, err);
    printf("RIP: 0x%016lx\nERR:0x%lx\n");
    printf("Error from: ");
    if (err_code.external == 1) {
        printf("External\n");
    }
    if (err_code.tbl == 0) {
        printf("GDT\n");
    } else if (err_code.tbl == 1 || err_code.tbl == 3) {
        printf("IDT\n");
    } else if (err_code.tbl == 2) {
        printf("LDT\n");
    }
    printf("index: 0x%lx\n", err_code.index);
    printf("abort !\n");
    abort();
}