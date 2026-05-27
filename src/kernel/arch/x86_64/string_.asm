section .text

global kmemset32
kmemset32:
    push rdi
    mov rax, rsi
    mov rcx, rdx
    rep stosd
    pop rax
    ret
    
global kmemmove
kmemmove:
    mov rcx, rdx
    mov rax, rdi

    cmp rdi, rsi
    ja .copy_backwards

    rep movsb
    jmp .done

  .copy_backwards:
    lea rdi, [rdi+rcx-1]
    lea rsi, [rsi+rcx-1]
    std
    rep movsb
    cld

  .done:
    ret

global kmemcmp
kmemcmp:
    mov rcx, rdx
    repe cmpsb
    je .equal

    mov al, byte [rdi-1]
    sub al, byte [rsi-1]
    movsx rax, al
    jmp .done

  .equal:
    xor eax, eax

  .done:
    ret

global kstrcpy
; char* strcpy(char* dest, const char* src)
kstrcpy:
    push rdi            ; Save original dest for return value
    
    ; 1. Find the length (like strlen)
    mov rdi, rsi        ; Move src to rdi for scasb
    xor al, al          ; Looking for null terminator (0)
    mov rcx, -1         ; Set counter to max
    repne scasb         ; Scan until 0 found
    
    not rcx             ; rcx is now (length + 1)
    mov rdx, rcx        ; Store length in rdx
    
    ; 2. Copy string
    pop rdi             ; Restore original dest
    push rdi            ; Save again for return
    mov rcx, rdx        ; Move length back to rcx
    rep movsb           ; Move rcx bytes from rsi to rdi
    
    pop rax             ; Return the original dest address
    ret
