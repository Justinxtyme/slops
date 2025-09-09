//gdt.c
#include "gdt.h"

void gdt_init(void) {
    gdt[0] = (struct GDTEntry){0}; // null

    // 64-bit code segment
    gdt[1] = (struct GDTEntry){
        .limit_low    = 0x0000,
        .base_low     = 0x0000,
        .base_mid     = 0x00,
        .access       = 0x9A,   // P=1, DPL=0, S=1, Type=code|readable
        .flags_limit  = 0xA0,   // G=1, L=1, D=0, AVL=0, limit high=0
        .base_high    = 0x00
    };

    // Data segment
    gdt[2] = (struct GDTEntry){
        .limit_low    = 0x0000,
        .base_low     = 0x0000,
        .base_mid     = 0x00,
        .access       = 0x92,   // P=1, DPL=0, S=1, Type=data|writable
        .flags_limit  = 0xC0,   // G=1, L=0, D=1, AVL=0, limit high=0
        .base_high    = 0x00
    };

    gdt_descriptor.limit = sizeof(gdt) - 1;
    gdt_descriptor.base  = (uint64_t)&gdt;
}

void gdt_install(void) {
    __asm__ volatile ("lgdt %0" :: "m"(gdt_descriptor) : "memory");

    // Far jump to reload CS with selector 0x08 (64-bit code segment)
    __asm__ volatile (
        "pushq $0x08\n\t"                // code segment selector
        "leaq 1f(%%rip), %%rax\n\t"      // address to jump to
        "pushq %%rax\n\t"
        "lretq\n\t"                      // far return to reload CS
        "1:\n\t"
        "mov $0x10, %%ax\n\t"            // data segment selector
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        :
        :
        : "rax", "ax", "memory"
    );
}

struct GDTPtr gdt_descriptor;
