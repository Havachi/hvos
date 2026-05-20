extern gdt_ptr
global reload_gdt

reload_gdt:
	lgdt [rel gdt_ptr]

	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	push 0x08
	lea rax, [rel .flush]
	push rax
	retfq

.flush:
	ret

global reload_tss
reload_tss:
	mov ax, 0x28
	ltr ax
	ret

global load_tss
load_tss:
	mov ax, di
	ltr ax
	ret