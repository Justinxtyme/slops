#include "idt.h"
#include <stdint.h>

typedef unsigned long size_t;

struct idt_ptr idtr = {
    .limit = sizeof(idt) - 1,
    .base  = (uint64_t)&idt
};

struct idt_entry idt[256];


void zero_idt(void) {
    struct idt_entry* e = idt;
    for (int i = 0; i < 256; i++) {
        e[i].offset_low  = 0;
        e[i].selector    = 0;
        e[i].ist         = 0;
        e[i].type_attr   = 0;
        e[i].offset_mid  = 0;
        e[i].offset_high = 0;
        e[i].zero        = 0;
    }
}



int set_idt_entry(int vec, void* handler, uint16_t selector, uint8_t type_attr) {
    sfprint("Setting IDT entry : %d\n", vec);
    uint64_t addr = (uint64_t)handler;
    idt[vec].offset_low  = addr & 0xFFFF;
    idt[vec].selector    = selector;
    idt[vec].ist         = 0;
    idt[vec].type_attr   = type_attr;
    idt[vec].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vec].zero        = 0;
    sfprint("addr: %8\n", addr);
    return 0;
}

//  calls set_idt_entry() with each extern handler 
int set_all_idt(void) {
    zero_idt();
    // make an array of handler names of the externs. t
    void (*handler_array[])() = {isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,
    isr11,isr12,isr13,isr14,isr15,isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,isr24,
    isr25,isr26,isr27,isr28,isr29,isr30,isr31};
    
    // get the number of handlers
    int num_handlers = sizeof(handler_array) / sizeof(handler_array[0]);
    
    //serial_printf("isr0=%p b0=%02x\n", isr0, (volatile uint8_t)isr0);    
    set_idt_entry(0,  handler_array[0],  0x08, 0x8E);
    set_idt_entry(1,  handler_array[1],  0x08, 0x8E);
    set_idt_entry(2,  handler_array[2],  0x08, 0x8E);
    set_idt_entry(3,  handler_array[3],  0x08, 0x8E);
    set_idt_entry(4,  handler_array[4],  0x08, 0x8E);
    set_idt_entry(5,  handler_array[5],  0x08, 0x8E);
    set_idt_entry(6,  handler_array[6],  0x08, 0x8E);
    set_idt_entry(7,  handler_array[7],  0x08, 0x8E);
    set_idt_entry(8,  handler_array[8],  0x08, 0x8E);
    set_idt_entry(9,  handler_array[9],  0x08, 0x8E);
    set_idt_entry(10, handler_array[10], 0x08, 0x8E);
    set_idt_entry(11, handler_array[11], 0x08, 0x8E);
    set_idt_entry(12, handler_array[12], 0x08, 0x8E);
    set_idt_entry(13, handler_array[13], 0x08, 0x8E);
    set_idt_entry(14, handler_array[14], 0x08, 0x8E);
    set_idt_entry(15, handler_array[15], 0x08, 0x8E);
    set_idt_entry(16, handler_array[16], 0x08, 0x8E);
    set_idt_entry(17, handler_array[17], 0x08, 0x8E);
    set_idt_entry(18, handler_array[18], 0x08, 0x8E);
    set_idt_entry(19, handler_array[19], 0x08, 0x8E);
    set_idt_entry(20, handler_array[20], 0x08, 0x8E);
    set_idt_entry(21, handler_array[21], 0x08, 0x8E);
    set_idt_entry(22, handler_array[22], 0x08, 0x8E);
    set_idt_entry(23, handler_array[23], 0x08, 0x8E);
    set_idt_entry(24, handler_array[24], 0x08, 0x8E);
    set_idt_entry(25, handler_array[25], 0x08, 0x8E);
    set_idt_entry(26, handler_array[26], 0x08, 0x8E);
    set_idt_entry(27, handler_array[27], 0x08, 0x8E);
    set_idt_entry(28, handler_array[28], 0x08, 0x8E);
    set_idt_entry(29, handler_array[29], 0x08, 0x8E);
    set_idt_entry(30, handler_array[30], 0x08, 0x8E);
    set_idt_entry(31, handler_array[31], 0x08, 0x8E); 


    //call set_idt_entry() Example; "set_idt_entry(0, isr0, 0x08, 0x0E);" 
    // for (int i = 0; i < num_handlers; i++) {
    //     sfprint("Setting %d handlers\n", num_handlers);
    //     if (set_idt_entry(i, handler_array[i], 0x08, 0x8E) != 0) {
    //         sfprint("Error setting idt entry: %d\n", i);
    //         sfprint("%s", "Bailing!\n");
    //         return 1;
    //     }

    // }
    // return 0;
}

static inline uint64_t read_cr2(void) {
    uint64_t v; asm volatile ("mov %%cr2, %0" : "=r"(v)); return v;
}

static void dump_pf_reason(uint64_t err) {
    sfprint("  reason: %s, %s, %s, RSVD=%8, IFetch=%8\n",
        (err & 1) ? "protection" : "not-present",
        (err & 2) ? "write" : "read",
        (err & 4) ? "user" : "supervisor",
        (err >> 3) & 1,
        (err >> 4) & 1);
}

void isr_handler(uint64_t vec, uint64_t err, uint64_t rip) {
    sfprint("Interrupt: %8\n", vec);
    sfprint("RIP: %8\n", rip);
    sfprint("Error code: %8\n", err);
    if (vec == 14) {
        uint64_t cr2 = read_cr2();
        sfprint("CR2 (fault addr): %8\n", cr2);
        dump_pf_reason(err);
    }
    for (;;) __asm__ volatile ("hlt");
}



// void isr_handler(uint64_t vec, uint64_t err, uint64_t rip) {
//     sfprint("Interrupt: %8\n", vec);
//     sfprint("RIP: %8\n", rip);
//     sfprint("Error code: %8\n", err);
//     for(;;) __asm__ volatile("hlt");
// }



// __attribute__((interrupt))
// void dummy_handler(void* frame) {
//     // You can log something here or just halt
//     for (;;) __asm__ volatile ("hlt");
// }