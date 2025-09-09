/* FOR 64 BIT */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

/* Tell the linker where execution starts */
ENTRY(_start)

/* Define memory layout and section placement */
SECTIONS
{
    /* Load address for the kernel — GRUB loads it here by default */
    . = 0x100000;

    /* Multiboot header must be within the first part of the file and 8-byte aligned */
    .multiboot_header ALIGN(8) :
    {
        KEEP(*(.multiboot_header))  /* Ensure it's not discarded */
    }

    /* Kernel code (text section) */
    .text ALIGN(4K) :
    {
        *(.text) *(.text.*)
    }

    /* Read-only data */
    .rodata ALIGN(4K) :
    {
        *(.rodata) *(.rodata.*)
    }

    /* Initialized data */
    .data ALIGN(4K) :
    {
        *(.data) *(.data.*)
    }

    /* Uninitialized data (zeroed at runtime) */
    .bss ALIGN(4K) (NOLOAD) :
    {
        __bss_start__ = .;
        *(.bss)
        *(COMMON)
        __bss_end__ = .;
    }

    /* Dedicated stack region — not part of .bss zeroing */
    .stack ALIGN(16) (NOLOAD) :
    {
        _stack_bottom = .;        /* lower address */
        . = . + 0x4000;           /* 16 KiB */
        _stack_top = .;           /* higher address for RSP */
    }

    /* IDT Tables */
    .idt ALIGN(4k) : 
    {
        KEEP(*(.idt))
    }
    

    /* Optional: heap start marker */
    . = ALIGN(4K);
    _heap_start = .;
}