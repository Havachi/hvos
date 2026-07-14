extern schedule
global scheduler_isr_asm
global switch_to
global kernel_yield
extern pit_interrupt_exit

section .text

scheduler_isr_asm:
	cli
	test qword [rsp + 8], 3
	jz .from_kernel
	swapgs
.from_kernel:
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
	mov rsi, 0
	call schedule
	mov rsp, rax
	test qword [rsp + 8], 3
	jz .to_kernel
	swapgs
.to_kernel:
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
	sti
	iretq

switch_to:
	cmp rsi, 0
	je .no_switch

	mov [rdi], rsp
	mov rsp, [rsi]

.no_switch:
	ret



kernel_yield:
	cli
	mov rax, ss
	push rax
	mov rax, rsp
	add rax, 8
	push rax
	pushfq
	mov rax, cs
	push rax
	mov rax,[rsp + 32]
	push rax

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
	call schedule
	mov rsp, rax

  	jmp pit_interrupt_exit