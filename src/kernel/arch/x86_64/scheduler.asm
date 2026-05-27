global scheduler_isr_asm
extern scheduler_c
global context_switch

scheduler_isr_asm:
	push r15
	push r14
	push r13
	push r12
	push r11
	push r10
	push r9
	push r8
	push rbp
	push rdi
	push rsi
	push rdx
	push rcx
	push rbx
	push rax

	mov rdi, rsp
	call scheduler_c
	mov rsp, rax

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

	iretq

context_switch:
	push r15
	push r14
	push r13
	push r12
	push r11
	push r10
	push r9
	push r8
	push rbp
	push rdi
	push rsi
	push rdx
	push rcx
	push rbx
	push rax

	mov [rdi], rsp
	mov rsp, rsi

	test rdx, rdx
	jz .skip_mmu_swap
	mov rax, cr3
	cmp rax, rdx
	je .skip_mmu_swap
	mov cr3, rdx
.skip_mmu_swap:

	pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15