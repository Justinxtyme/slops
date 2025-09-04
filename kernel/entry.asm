BITS 32

section .multiboot_header align=8
header_start:
    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
    dw 0
    dw 0
    dd 8
header_end:

section .text align=16
global _start
extern kernel_main
extern __bss_start__
extern __bss_end__
extern _stack_top

%define DBGPORT   0x00E9
%define MB2_MAGIC 0x36d76289

_start:
    cli

    ; X = entered _start
    push    eax
    mov     al, 'X'
    mov     dx, DBGPORT
    out     dx, al
    pop     eax

    ; VGA poke (only visible in text mode)
    mov     dword [0xb8000], 0x07200758

    ; Check MB2 magic
    cmp     eax, MB2_MAGIC
    jne     .mb_bad

    ; 2 = magic OK
    push    eax
    mov     al, '2'
    mov     dx, DBGPORT
    out     dx, al
    pop     eax

    ; Zero .bss
    cld
    mov     edi, __bss_start__
    mov     ecx, __bss_end__
    sub     ecx, edi
    xor     eax, eax
    rep     stosb

    ; Set stack after zeroing
    mov     esp, _stack_top

    ; K = calling kernel_main
    mov     al, 'K'
    mov     dx, DBGPORT
    out     dx, al

    push    ebx
    call    kernel_main

    ; R = returned from C
    mov     al, 'R'
    mov     dx, DBGPORT
    out     dx, al

.halt:
    hlt
    jmp     .halt

.mb_bad:
    ; 1 = magic mismatch
    mov     al, '1'
    mov     dx, DBGPORT
    out     dx, al
    mov     word [0xb8002], 0x0731
    jmp     .halt

section .note.GNU-stack noalloc noexec nowrite progbits