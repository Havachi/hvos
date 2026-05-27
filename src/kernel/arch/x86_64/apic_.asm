default rel

global rdmsr
rdmsr:
    mov ecx, edi
    rdmsr
    shl rdx, 32
    or rax, rdx
    ret

global wrmsr
wrmsr:
    mov ecx, edi
    mov rax, rsi
    mov rdx, rsi
    shr rdx, 32
    wrmsr
    ret

extern scheduler_c
global timer_handler_asm

timer_handler_asm:
    cmp qword [rsp + 8], 0x2B
    jne .skip_swapgs_entry
    swapgs
.skip_swapgs_entry:
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
	call scheduler_c
	mov rsp, rax
    
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

    cmp qword [rsp + 8], 0x2B
    jne .skip_swapgs_exit
    swapgs

.skip_swapgs_exit:
    iretq