extern scheduler_c

global scheduler_isr_asm
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
	mov ax, 0x1B
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

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


global context_switch
context_switch:
	push rbx
	push rsi
	push rdi
	push rbp

	mov [rdi], rsp
	mov rsp, rsi

	pop rbp
	pop rdi
	pop rsi
	pop rbx

	ret

global new_task_setup
new_task_setup:
	pop rbx
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	xor rax, rax
	xor rbx, rbx
	xor rcx, rcx
	xor rdx, rdx
	xor rsi, rsi
	xor rdi, rdi
	xor rbp, rbp

	iretq