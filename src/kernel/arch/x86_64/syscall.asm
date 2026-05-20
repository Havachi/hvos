[bits 64]
extern syscall_handler
extern global_tss
extern syscall_kernel_stack

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

	push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

	mov rdi, rsp
	call syscall_handler

	pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 8
    add rsp, 8
    add rsp, 8
    pop r10
    add rsp, 8
    mov r11, 0x202
    mov rsp, r10

	swapgs
	o64 sysret
