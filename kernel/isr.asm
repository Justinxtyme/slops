global isr_common_stub
extern isr_handler

; Common stub preserves state and calls isr_handler(rdi=vec, rsi=err, rdx=rip)
isr_common_stub:
    ; Save volatile we’ll clobber (don’t touch rdi/rsi/rdx – they are args)
    push r11
    push r10
    push r9
    push r8
    push rcx
    push rax

    ; Align stack for SysV: at call, (rsp % 16) == 8
    sub rsp, 8
    call isr_handler
    add rsp, 8

    ; Restore
    pop rax
    pop rcx
    pop r8
    pop r9
    pop r10
    pop r11

    ; Return to interrupted context
    iretq

%macro ISR_NOERR 1
global isr%1
isr%1:
    ; rdi=vector, rsi=0, rdx = RIP at [rsp]
    mov rdi, %1
    xor rsi, rsi
    mov rdx, [rsp]          ; RIP (top of frame)
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    ; rdi=vector, rsi=error at [rsp], rdx = RIP at [rsp+8]
    mov rdi, %1
    mov rsi, [rsp]          ; error code pushed by CPU
    mov rdx, [rsp+8]        ; RIP (next qword)
    jmp isr_common_stub
%endmacro

; 0..31
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31