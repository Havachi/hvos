global check_rdrand_support
global rand32
global seed_prng 

section .data
align 8
rng_state: dq 0xCFEF097F689210AE ;should be randomized at boot

section .text

;int check_rdrand_support(void);
check_rdrand_support:
	push rbx
	mov eax, 1
	cpuid

	bt ecx, 30
	setc al
	movzx eax, al
	pop rbx
	ret

seed_prng:
	mov [rng_state], rdi
	ret

rand32:
	push rdi
	call check_rdrand_support
	pop rdi

	test eax, eax
	jz	.fallback_prng

	rdrand eax
	jnc	.fallback_prng

	mov dword [rdi], eax
	mov	eax, 1
	ret

.fallback_prng:
	mov rax, [rng_state]
	mov rcx, 0x9e3779b97f4a7c15
	add rax, rcx
	mov [rng_state], rax

	mov rcx, rax
	shr rcx, 30
	xor rax, rcx
	mov	rcx, 0xbf58476d1ce4e5b9
	imul rax, rcx

	mov rcx, rax
	shr rcx, 27
	xor rax, rcx
	mov rcx, 0x94d049bb133111eb
	imul rax, rcx

	mov rcx, rax
	shr rcx, 31
	xor rax, rcx

	mov dword [rdi], eax
	mov eax, 1
	ret