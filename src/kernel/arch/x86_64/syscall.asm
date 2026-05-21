[bits 64]
extern syscall_handler
extern global_tss
extern syscall_kernel_stack
extern current_task_syscall_stack
global syscall_entry_asm


syscall_entry_asm:
	swapgs
	mov r10, rsp

	mov rsp, [rel syscall_kernel_stack];

	push 0x1B
	push r10
	push 0x202
	push 0x23
	push rcx

	push r15
	push r14
	push r13
	push r14
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
	and rsp, ~0xF

	mov rdi, rbp
	call syscall_handler
	mov rsp, rbp
	cli
	
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
	add rsp, 8
	pop r13
	pop r14
	pop r15

    add rsp, 8
    add rsp, 8
    add rsp, 8
    pop r10
    add rsp, 8
    mov r11, 0x202
    mov rsp, r10

	swapgs
	o64 sysret
