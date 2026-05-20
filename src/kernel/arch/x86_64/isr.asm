%macro pushall 0

push rax
push rbx
push rcx
push rdx
push rbp
push rdi
push rsi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15

%endmacro

%macro popall 0

pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rsi
pop rdi
pop rbp
pop rdx
pop rcx
pop rbx
pop rax

%endmacro

extern isr_common_handler

; Common handler for the ISRs
isr_common_format:
	pushall
	mov rdi, rsp
	mov rbx, rsp
	and rsp, -16
	call isr_common_handler
	mov rsp, rbx
	popall
	add rsp, 16
	iretq

%macro isr 1

global isr%1
isr%1:
	push 0
	push %1
	jmp isr_common_format

%endmacro

%macro error_isr 1

global isr%1
isr%1:
	push %1
	jmp isr_common_format

%endmacro

%define has_errcode(i) (i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 21 || i == 29 || i == 30)

; Define ISRs
%assign i 0
%rep 256
%if !has_errcode(i)
	isr i
%else
	error_isr i
%endif
%assign i i + 1
%endrep


global keyboard_handler_asm
extern keyboard_handler_c

keyboard_handler_asm:
	pushall
	call keyboard_handler_c
	popall
	iretq