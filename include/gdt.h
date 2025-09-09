//gdt.h
#ifndef GDT_H 
#define GDT_H

#include <stdint.h>  

// Structure representing one GDT entry (8 bytes total in this layout)
struct GDTEntry {
    uint16_t limit_low;     // Lower 16 bits of the segment limit
    uint16_t base_low;      // Lower 16 bits of the base address
    uint8_t  base_mid;      // Middle 8 bits of the base address
    uint8_t  access;        // Access flags: present bit, privilege level, type (code/data)
    uint8_t  flags_limit;   // Upper 4 bits of limit + flags (granularity, size, long mode)
    uint8_t  base_high;     // Upper 8 bits of the base address
} __attribute__((packed));  // Prevent compiler from adding padding between fields

// Structure for the GDTR register (pointer to the GDT)
struct GDTPtr {
    uint16_t limit;         // Size of the GDT in bytes minus 1
    uint64_t base;          // 64-bit linear address of the first GDT entry
} __attribute__((packed));  // Must be packed so lgdt reads the correct layout

// Our actual GDT table: 3 entries (null, code, data)
static struct GDTEntry gdt[3];


// The descriptor we’ll load into GDTR with lgdt
extern struct GDTPtr gdt_descriptor;



void gdt_init(void);

void gdt_install(void);

#endif