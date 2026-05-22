extern gdt_ptr
extern _kernel_cs
extern _kernel_ds
extern _user_cs
extern _user_ds
extern _tss_seg

global reload_gdt

reload_gdt:
	lgdt [rel gdt_ptr] 

	mov rax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	push _kernel_cs
	lea rax, [rel .flush]
	push rax
	retfq

.flush:
	ret

global reload_tss
reload_tss:
	mov ax, [rel _tss_seg]
	ltr ax
	ret

global load_tss
load_tss:
	mov ax, di
	ltr ax
	ret