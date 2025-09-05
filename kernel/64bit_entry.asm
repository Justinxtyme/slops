; entry.asm -- Multiboot2 header + 32-bit -> 64-bit long mode trampoline
; Fully commented: every instruction explained.
;
; Assemble:
;   nasm -f elf64 entry.asm -o entry.o
; Link (example):
;   x86_64-elf-ld -nostdlib -T linker.ld entry.o kernel_main.o -o kernel.elf
;
; Build notes at the bottom.

%define DBGPORT     0x00E9          ; QEMU debug port (useful for quick debug chars)
%define MB2_MAGIC   0x36d76289      ; expected Multiboot2 magic in EAX (from GRUB)
%define MSR_EFER    0xC0000080      ; MSR index for EFER register
%define EFER_LME    (1 << 8)        ; EFER.LME bit (enable long mode)
%define CR0_PE      (1 << 0)        ; CR0.PE (protected mode) - already set by GRUB
%define CR0_PG      (1 << 31)       ; CR0.PG (enable paging)
%define CR4_PAE     (1 << 5)        ; CR4.PAE (Physical Address Extension) - required for long mode
%define PAGE_P      (1 << 0)        ; page present
%define PAGE_RW     (1 << 1)        ; page writable
%define PAGE_PS     (1 << 7)        ; page size (1 -> 2MiB in PD entry)

; ---------------------------------------------------------------------------------
; 32-bit entry (GRUB gives us control in 32-bit protected mode).
; We'll remain in 32-bit until the far jump to the 64-bit CS selector.
; ---------------------------------------------------------------------------------

BITS 32

; --------------------------
; Multiboot2 header (aligned)
; Must be present early in the file and aligned to 8 bytes.
; The checksum field is set so that sum(header fields) == 0 (mod 2^32).
; --------------------------
section .multiboot_header align=8
header_start:
    dd 0xE85250D6                ; multiboot2 magic
    dd 0                        ; architecture (0 == i386 for MB2)
    dd header_end - header_start ; header length in bytes
    ; checksum = -(magic + arch + length)
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start))
    ; end tag (type=0, size=8)
    dw 0
    dw 0
    dd 8
header_end:

; --------------------------
; Code and definitions
; --------------------------
section .text align=16
global _start
extern kernel_main        ; C function (will be compiled as 64-bit)
extern __bss_start__      ; linker-provided symbol (start of .bss)
extern __bss_end__        ; linker-provided symbol (end of .bss)
extern _stack_top         ; linker symbol: top of kernel stack (high address)

; Entry from GRUB -> we start here in 32-bit protected mode
_start:
    cli                     ; clear interrupts while we set up critical state

    ; ---------- Debug: write 'X' to QEMU's debug port (0xE9) ----------
    ; Useful to know we've reached this point (QEMU prints to host console).
    mov al, 'X'             ; ASCII character 'X' into AL
    mov dx, DBGPORT         ; DX = port (0xE9)
    out dx, al              ; OUT to port DX: write AL

    ; ---------- Optional VGA poke for visibility in text mode ----------
    ; Write a dword directly to VGA text memory at 0xb8000 to show something.
    ; This is safe because we identity-map that address later.
    mov dword [0xb8000], 0x07200758
    ; Explanation: writes characters/attributes at top-left of screen; not important

    ; ---------- Verify Multiboot2 magic that GRUB placed in EAX ----------
    ; Per Multiboot2 spec, GRUB leaves magic in EAX. We check it.
    cmp eax, MB2_MAGIC      ; compare EAX with expected magic
    jne .mb_bad             ; if not equal, go error path

    ; ---------- Debug: '2' == multiboot2 OK ----------
    mov al, '2'
    mov dx, DBGPORT
    out dx, al

    ; ---------- Clear .bss (uninitialized data) ----------
    ; BSS must be zeroed before use. Linker defined __bss_start__/__bss_end__.
    cld                     ; clear direction flag so stos increments EDI
    mov edi, __bss_start__  ; EDI = start address of BSS
    mov ecx, __bss_end__    ; ECX = end address (temporarily)
    sub ecx, edi            ; ECX = size of BSS in bytes
    xor eax, eax            ; AL/AX/EAX = 0 (value to store)
    rep stosb               ; store AL into [EDI] ECX times -> zeroes BSS

    ; ---------- Set a temporary 32-bit stack (we'll switch to 64-bit RSP later) ----------
    mov esp, _stack_top     ; ESP = top of stack region (linker defines this)

    ; ---------- Build page tables for long mode (identity map first 1 GiB) ----------
    ; We implement setup_paging_1g as a 32-bit routine that fills PML4/PDPT/PD entries.
    call setup_paging_1g

    ; ---------- Load the GDT which contains a 64-bit code selector ----------
    ; lgdt expects a memory operand containing a 16-bit limit and 32-bit base (in 32-bit).
    lgdt [gdt64_ptr]        ; load GDT pointer (limit + base) from memory

    ; ---------- Enable PAE (Physical Address Extension) in CR4 ----------
    ; PAE is required for long mode page tables (PML4).
    mov eax, cr4            ; read CR4 into EAX (32-bit)
    or  eax, CR4_PAE        ; set the PAE bit (bit 5)
    mov cr4, eax            ; write back CR4

    ; ---------- Load CR3 with the physical base address of the PML4 ----------
    ; On x86-64 CR3 points to the PML4 table. Here we use physical addresses
    ; within the first GiB, so identity-mapped physical == virtual.
    mov eax, pml4_table     ; put base address of pml4_table (physical) into EAX
    mov cr3, eax            ; set CR3 = physical address of PML4

    ; ---------- Enable Long Mode via MSR EFER.LME ----------
    ; rdmsr/wrmsr use ECX = MSR index, result in EDX:EAX.
    mov ecx, MSR_EFER       ; ECX = MSR index for EFER
    rdmsr                  ; EDX:EAX = MSR[ECX] (read EFER)
    or  eax, EFER_LME       ; set bit 8 (LME)
    wrmsr                  ; write back EDX:EAX to MSR[ECX]

    ; ---------- Enable paging (CR0.PG) ----------
    ; GRUB already set CR0.PE (protected mode). Now set CR0.PG to enable paging.
    mov eax, cr0            ; read CR0
    or  eax, CR0_PG         ; set PG bit
    mov cr0, eax            ; write back CR0 -> paging now enabled
    ; At this point, because CR4.PAE and EFER.LME are set and CR0.PG is set,
    ; the CPU transitions into long mode (if supported) — but execution is still
    ; using the 32-bit CS. The far jump to a 64-bit code selector finishes the switch.

    ; ---------- Far jump into 64-bit code segment (selector 0x08) ----------
    ; This performs a far jump: loads CS = 0x08 and instruction pointer = long_mode_start.
    ; Selector 0x08 is defined in our GDT as the 64-bit code segment.
    jmp 0x08:long_mode_start

; -----------------------
; Multiboot error path
; -----------------------
.mb_bad:
    mov al, '1'
    mov dx, DBGPORT
    out dx, al
    mov word [0xb8002], 0x0731    ; show '1' on screen top-left
.halt:
    hlt
    jmp .halt

; ---------------------------------------------------------------------------------
; LONG MODE ENTRY (64-bit instructions from here)
; After far jmp above, the CPU executes in long mode with CS=0x08 (the 64-bit CS).
; ---------------------------------------------------------------------------------

BITS 64
extern _heap_start        ; optional heap marker from linker (64-bit symbol)

long_mode_start:
    ; ---------------- Set up data segment registers ----------------
    ; In long mode, segmentation is mostly disabled for data. Still we must
    ; load DS/ES/SS/etc with a valid selector (0x10 -> data selector in GDT).
    mov ax, 0x10           ; selector for data segment (match second GDT entry)
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; ---------------- Set 64-bit stack pointer ----------------
    ; _stack_top is defined by the 64-bit linker script and points to highest stack address.
    mov rsp, _stack_top    ; set RSP to the top of the stack (stack grows downward)

    ; ---------- Debug: 'K' before calling kernel_main ----------
    mov al, 'K'
    mov dx, DBGPORT
    out dx, al

    ; ---------- Prepare first argument and call kernel_main (SysV ABI) ----------
    ; SysV AMD64 ABI: first arg in RDI. If you want to pass multiboot info,
    ; move it into RDI here. We didn't preserve EBX earlier; adjust if needed.
    xor rdi, rdi           ; set first arg (RDI) = 0; change if you pass a pointer
    call kernel_main       ; call into your 64-bit C kernel entry

    ; ---------- Debug: 'R' after kernel_main returns ----------
    mov al, 'R'
    mov dx, DBGPORT
    out dx, al

.halt64:
    hlt
    jmp .halt64

; ---------------------------------------------------------------------------------
; Page tables (in .bss, 4KiB-aligned).
; We create three tables:
;   - pml4_table  (512 entries, each 8 bytes)   -> top level in x86-64
;   - pdpt_table  (512 entries)                 -> PDPT
;   - pd_table    (512 entries)                 -> Page Directory using 2MiB pages
; We'll identity-map the first 1 GiB using PD entries of 2MiB (512 * 2MiB = 1GiB).
; ---------------------------------------------------------------------------------

ALIGN 4096
section .bss
align 4096
pml4_table:  resq 512      ; reserve 512 * 8 = 4096 bytes
pdpt_table:  resq 512
pd_table:    resq 512      ; will contain 2MiB-page entries (512 entries cover 1 GiB)

section .text
; ------------------------------------------------------------
; setup_paging_1g:
;   Creates PML4[0] -> PDPT, PDPT[0] -> PD, and fills PD with 2MiB pages
;   Each page-table entry is 64 bits. While still in 32-bit mode we write
;   each 64-bit entry as two 32-bit writes: low dword then high dword.
; ------------------------------------------------------------
setup_paging_1g:
    ; Preserve registers we'll use (callee-saved convention not required here,
    ; but we push/pop to avoid disturbing EDI/ECX used elsewhere).
    push ebx
    push esi
    push edi
    push ecx

    ; Zero PML4 (512 qwords -> 4096 bytes)
    mov edi, pml4_table        ; EDI points to PML4 base
    mov ecx, 512               ; number of qwords to zero
.zero_pml4_loop:
    mov dword [edi], 0         ; write low 32 bits = 0
    mov dword [edi+4], 0       ; write high 32 bits = 0
    add edi, 8
    loop .zero_pml4_loop

    ; Zero PDPT
    mov edi, pdpt_table
    mov ecx, 512
.zero_pdpt_loop:
    mov dword [edi], 0
    mov dword [edi+4], 0
    add edi, 8
    loop .zero_pdpt_loop

    ; Zero PD
    mov edi, pd_table
    mov ecx, 512
.zero_pd_loop:
    mov dword [edi], 0
    mov dword [edi+4], 0
    add edi, 8
    loop .zero_pd_loop

    ; Now link PML4[0] -> PDPT:
    ; PML4[0] entry = physical_base_of(pdpt_table) | flags
    ; compute low dword (address & 0xFFFFFFFF) into EBX, zero high dword
    mov ebx, pdpt_table        ; EBX = base address of PDPT
    ; low dword = EBX | flags (PAGE_P | PAGE_RW)
    or ebx, PAGE_P | PAGE_RW
    mov dword [pml4_table], ebx    ; store low 32 bits
    mov dword [pml4_table + 4], 0  ; store high 32 bits (zero)

    ; Link PDPT[0] -> PD:
    mov ebx, pd_table
    or ebx, PAGE_P | PAGE_RW
    mov dword [pdpt_table], ebx
    mov dword [pdpt_table + 4], 0

    ; Fill PD with 2MiB pages:
    ; PD[i] = (i * 2MiB) | flags (PAGE_P | PAGE_RW | PAGE_PS)
    xor ebx, ebx                ; EBX = i (page index)
.fill_pd_loop_32:
    ; compute low dword: (i * 2MiB)
    mov eax, ebx                ; EAX = i
    shl eax, 21                 ; EAX = i * 2^21 = i * 2MiB
    or eax, (PAGE_P | PAGE_RW | PAGE_PS) ; set flags in low dword
    mov dword [pd_table + ebx*8], eax   ; store low 32 bits into PD[i]
    mov dword [pd_table + ebx*8 + 4], 0 ; high 32 bits = 0 (addresses < 4GiB)

    inc ebx
    cmp ebx, 512
    jb .fill_pd_loop_32

    ; restore saved registers
    pop ecx
    pop edi
    pop esi
    pop ebx
    ret

; ---------------------------------------------------------------------------------
; Minimal 64-bit GDT
; We prepare three descriptors:
;   0x00: null descriptor
;   0x08: 64-bit code segment (L bit set)
;   0x10: data segment
; gdt64_ptr is a 6/10-byte LGDT descriptor: 16-bit limit, 32-bit/64-bit base.
; For LGDT in 32-bit mode we supply a 6-byte structure: word limit, dword base.
; ---------------------------------------------------------------------------------

ALIGN 8
section .rodata
gdt64:
    dq 0x0000000000000000     ; 0x00: null descriptor (8 bytes)
    ; 0x08: code descriptor: flags set for 64-bit code (L=1), present, DPL=0
    ; Layout assembled as QWORD: see Intel docs; this constant sets:
    ;   base = 0, limit = 0xFFFFF, G = 1 (granularity), L = 1 (64-bit)
    dq 0x00AF9A000000FFFF
    ; 0x10: data descriptor (flat data)
    dq 0x00AF92000000FFFF

; gdt descriptor for lgdt instruction (in 32-bit: 6 bytes: word limit + dword base)
gdt64_ptr:
    dw gdt64_end - gdt64 - 1  ; 16-bit limit = size - 1
    dd gdt64                  ; 32-bit base address (low part)
gdt64_end:

; ---------------------------------------------------------------------------------
; Marker for non-executable stack (linker note)
; ---------------------------------------------------------------------------------
section .note.GNU-stack noalloc noexec nowrite progbits

; ---------------------------------------------------------------------------------
; END OF FILE
; ---------------------------------------------------------------------------------

; --------------------------
; BUILD / ASSEMBLE / LINK NOTES
; --------------------------
; 1) Assembler:
;    nasm -f elf64 entry.asm -o entry.o
;
; 2) Compiler for kernel C (example):
;    x86_64-elf-gcc -ffreestanding -fno-stack-protector -fno-pic \
;                   -mno-red-zone -mcmodel=kernel -O2 -c kernel_main.c -o kernel_main.o
;    -fno-builtin or -nostdlib as needed
;
; 3) Linker script: ensure OUTPUT_FORMAT(elf64-x86-64) and ENTRY(_start)
;    and that _stack_top / __bss_start__ / __bss_end__ are defined there.
;
; 4) Link:
;    x86_64-elf-ld -nostdlib -T linker.ld entry.o kernel_main.o -o kernel.elf
;
; 5) GRUB:
;    multiboot2 /boot/kernel.elf
;    (grub.cfg does not need special changes; GRUB loads you in 32-bit— you switch to long mode)
;
; 6) Notes / caveats:
;    - We use identity mappings for addresses < 1GiB. That means physical == virtual for that region.
;    - If you pass Multiboot info pointer (MB2 info) to kernel_main, preserve EBX before
;      any code that clobbers it and move it into RDI before calling kernel_main in 64-bit.
;    - rdmsr/wrmsr and CR register writes require ring 0 (OK in kernel).
;    - If you plan to support >4GiB physical, you'll need to handle high bits when writing table entries.
;    - The GDT entry constants used work for a flat 64-bit code/data mapping; study the bits if you modify.