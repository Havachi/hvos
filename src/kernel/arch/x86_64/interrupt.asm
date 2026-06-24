[BITS 64]
default rel	
global default_exception_handler
global default_interrupt_handler
global exception_handlers
global pit_interrupt
global spurious_interrupt
global keyboard_interrupt
global pagefault_interrupt
global gpf_exc
global generic_exeption_handler

extern g_pit_ticks
extern g_local_apic_address
extern exception_dump
extern schedule
extern keyboard_handler_c
extern page_fault_handler_c
extern gpf_execption_handler_c
extern update_cursor
extern update_curr_thread
extern context_switch

default_exception_handler:
	jmp $

default_interrupt_handler:
	iretq

%macro make_exception_handler 1
exception%1_handler:
	cli
	push byte 0
	push byte %1
	jmp exception_body
%endmacro

%macro make_error_exception_handler 1
exception%1_handler:
	cli
	push byte %1
	jmp exception_body
%endmacro

make_exception_handler 0
make_exception_handler 1
make_exception_handler 2
make_exception_handler 3
make_exception_handler 4
make_exception_handler 5
make_exception_handler 6
make_exception_handler 7
make_error_exception_handler 8
make_exception_handler 9
make_error_exception_handler 10
make_error_exception_handler 11
make_error_exception_handler 12
make_error_exception_handler 13
make_error_exception_handler 14
make_exception_handler 15
make_exception_handler 16
make_error_exception_handler 17
make_exception_handler 18
make_exception_handler 19

exception_handlers:
	dq exception0_handler
	dq exception1_handler
	dq exception2_handler
	dq exception3_handler
	dq exception4_handler
	dq exception5_handler
	dq exception6_handler
	dq exception7_handler
	dq exception8_handler
	dq exception9_handler
	dq exception10_handler
	dq exception11_handler
	dq exception12_handler
	dq exception13_handler
	dq exception14_handler
	dq exception15_handler
	dq exception16_handler
	dq exception17_handler
	dq exception18_handler
	dq exception19_handler

exception_body:
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi

	call exception_dump

	jmp $

pit_interrupt:
	test qword [rsp + 8],3
	jz .from_kernel
	swapgs
.from_kernel:
	cli
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

	inc qword [rel g_pit_ticks]
	mov rax, [rel g_local_apic_address]
	mov dword [rax + 0xB0], 0

	call update_cursor
	call update_curr_thread

	mov rdi, rsp
	mov rsi, 1
	call schedule
	mov rsp, rax

global pit_interrupt_exit
pit_interrupt_exit:
	test qword [rsp + 128], 3
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
	
keyboard_interrupt:
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

	call keyboard_handler_c
	mov rax, [rel g_local_apic_address]
	mov dword [rax + 0xB0], 0
	
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

pagefault_interrupt:
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

    mov rdi, rsp        ; arg1: fault_frame_t*
    mov rax, cr2
    mov rsi, rax        ; arg2: cr2

    call page_fault_handler_c

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

    add rsp, 8          ; pop error code
    iretq

gpf_exc:
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

	mov rsi, [rsp + 120]
	mov rdi, [rsp + 128]

	call gpf_execption_handler_c

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
	iretq


generic_exeption_handler:
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

	call exception_dump

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


spurious_interrupt:
	iretq