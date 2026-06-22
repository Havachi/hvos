[bits 64]
extern syscall_handler
extern global_tss
extern syscall_kernel_stack
extern current_task_syscall_stack
global syscall_entry_asm

%define CPU_SCRATCH_SP2 0
%define CPU_STACK_TOP 8

syscall_entry_asm:
	cli
	swapgs
	mov [gs:CPU_SCRATCH_SP2], rsp
	mov rsp, [gs:CPU_STACK_TOP]

	; User SS
	push 0x1B
	; User RSP 
	push qword [gs:CPU_SCRATCH_SP2]
	; User RFLAGS
	push r11
	; User CS
	push 0x23
	; User RIP
	push rcx
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

	sti
	mov rbp, rsp

	mov rdi, rbp
	call syscall_handler
	cli
	mov rsp, rbp

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
	add rsp, 8

	pop rcx
	add rsp, 8
	pop r11

	mov rsp, [gs:CPU_SCRATCH_SP2]
	swapgs
	o64 sysret

