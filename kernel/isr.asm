global isr_common_stub
extern isr_handler

; For no-error vectors, push a synthetic 0 error code to unify layout
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0             ; err = 0
    push qword %1            ; vec
    jmp isr_common_stub
%endmacro

; For error-code vectors, CPU already pushed err; we push vec to unify
%macro ISR_ERR 1
global isr%1
isr%1:
    push qword %1            ; vec
    jmp isr_common_stub
%endmacro

; Common stub: build frame, call C, restore, iretq
isr_common_stub:
    ; Save all GPRs (callee and caller saved)
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

    ; RSP alignment:
    ; On interrupt entry, RSP ≡ +8 mod 16 relative to interrupted context.
    ; We just pushed 15 regs = 120 bytes (≡ 8 mod 16). Plus the 2 qwords we pushed
    ; before (err/vec) = 16 bytes total (≡ 0). Net so far: +8. That leaves RSP ≡ 8 mod 16.
    ; Perfect for a CALL (CALL pushes 8 to make callee entry 16-aligned).
    ; So do NOT sub rsp, 8.

    ; Pass pointer to our frame to C in RDI
    mov rdi, rsp

    call isr_handler

    ; Restore all GPRs in reverse
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

    ; Pop vec and err we pushed (or err+vec depending on path)
    add rsp, 16

    iretq


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
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
