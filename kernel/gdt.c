//gdt.c
#include "gdt.h"

void gdt_init(void) {
    // ---------------------------
    // Null descriptor (selector 0x00)
    // Required by x86 — using selector 0x00 will cause a fault
    gdt[0] = (struct GDTEntry){0};  // Zero-initialize all fields

    // ---------------------------
    // Code segment descriptor (selector 0x08)
    gdt[1] = (struct GDTEntry){
        .limit_low    = 0x0000, // Limit ignored in long mode (paging handles bounds)
        .base_low     = 0x0000, // Base ignored in long mode (flat addressing)
        .base_mid     = 0x00,   // Middle byte of base (ignored in long mode)
        .access       = 0x9A,   // 1 00 1 1 010: present=1, DPL=0, code=1, readable=1
        .flags_limit  = 0x20,   // 0010 0000: L=1 (64-bit), D=0, G=0, plus high limit bits=0
        .base_high    = 0x00    // High byte of base (ignored in long mode)
    };

    // ---------------------------
    // Data segment descriptor (selector 0x10)
    gdt[2] = (struct GDTEntry){
        .limit_low    = 0x0000, // Limit ignored in long mode
        .base_low     = 0x0000, // Base ignored in long mode
        .base_mid     = 0x00,   // Middle byte of base
        .access       = 0x92,   // 1 00 1 0 010: present=1, DPL=0, data=1, writable=1
        .flags_limit  = 0x00,   // L=0 for data segments, D=0, G=0, high limit bits=0
        .base_high    = 0x00    // High byte of base
    };

    // ---------------------------
    // Fill GDTR descriptor
    gdt_descriptor.limit = sizeof(gdt) - 1; // Total size of GDT in bytes minus 1
    gdt_descriptor.base  = (uint64_t)&gdt;  // 64-bit address of our GDT table
}

void gdt_install(void) {
    __asm__ __volatile__("lgdt %0" : : "m"(gdt_descriptor) : "memory");
    __asm__ __volatile__(
        "mov $0x10, %%ax \n\t"
        "mov %%ax, %%ds \n\t"
        "mov %%ax, %%es \n\t"
        "mov %%ax, %%ss \n\t"
        :
        :
        : "ax", "memory"
    );
}

struct GDTPtr gdt_descriptor;
