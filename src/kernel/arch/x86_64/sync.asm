global atomic_exchange

atomic_exchange:
	mov rax, rsi
	lock xchg [rdi], eax
	ret