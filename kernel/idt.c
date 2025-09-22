#include "idt.h"
#include "kbd.h"
#include "shell.h"
#include "assertf.h"
#include <stdint.h>

typedef unsigned long size_t;


struct idt_ptr idtr = {
    .limit = sizeof(idt) - 1,
    .base  = (uint64_t)&idt
};

// INIT  STRUCT FOR STORING IDT ENTRIES
struct idt_entry idt[256];

//INIT STRUCT FOR KEYBOARD RING BUFFER
static kbuff_t kbuff = {0};


static inline void irq_disable(void){ __asm__ __volatile__("cli"); }
static inline void irq_enable(void){ __asm__ __volatile__("sti"); }

int read_sc(ShellContext *shell) {
    irq_disable();
    uint16_t head_snapshot = kbuff.head;
    irq_enable();

    while (1) {
        irq_disable();
        uint16_t tail = kbuff.tail;
        if (tail == head_snapshot) { irq_enable(); break; }
        uint8_t sc = kbuff.scancodes[tail & KBDBUF_MASK];
        kbuff.tail = tail + 1;
        irq_enable();

        process_scancode(shell, sc); // may draw, safe with IRQs on
    }
    uint32_t used = kbuff.head - kbuff.tail;
    assertf(used <= KBDBUFFSIZE);
    return 0;
}



// int read_sc(ShellContext *shell) {
//     if (kbuff.tail == kbuff.head) return 0; //  buffer is empty
//     while (kbuff.tail != kbuff.head) {
//         //sfprint("Reading sc: %8\n", kbuff.tail);
//         uint8_t sc = kbuff.scancodes[kbuff.tail]; // pull scan code from tail.
//         kbuff.tail = (kbuff.tail + 1) % KBDBUFFSIZE; // move tail up, safely wrapping if > buff size;
//         //sfprint("Tail is now: %8\n", kbuff.tail);
//         // if (sc & 0x80) {
//         //     continue;
//         // }
//         process_scancode(shell, sc);
//     } 
//     return 0;   
// }

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
    //sfprint("Setting IDT entry : %d\n", vec);
    uint64_t addr = (uint64_t)handler;
    idt[vec].offset_low  = addr & 0xFFFF;
    idt[vec].selector    = selector;
    idt[vec].ist         = 0;
    idt[vec].type_attr   = type_attr;
    idt[vec].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vec].zero        = 0;
    //sfprint("addr: %8\n", addr);
    return 0;
}

//  calls set_idt_entry() with each extern handler 
int set_all_idt(void) {
    zero_idt();
    // make an array of handler names of the externs. t
    void (*handler_array[])() = {isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,
    isr11,isr12,isr13,isr14,isr15,isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,isr24,
    isr25,isr26,isr27,isr28,isr29,isr30,isr31,isr32,isr33,isr34,isr35,isr36};
    
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
    set_idt_entry(32, handler_array[32], 0x08, 0x8E); 
    set_idt_entry(33, handler_array[33], 0x08, 0x8E); 
    set_idt_entry(34, handler_array[34], 0x08, 0x8E);
    set_idt_entry(35, handler_array[35], 0x08, 0x8E);
    set_idt_entry(36, handler_array[36], 0x08, 0x8E);
    
    return 0;
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


void isr_handler(isr_frame_t* f) {
    switch (f->vec) {
        case 14: {
            uint64_t cr2 = read_cr2();
            dump_pf_reason(f->err);
            break;
        }
        case 32: irq0_handler(f); break;
        case 33: irq1_handler(f); break;
        default: break;
    }
    uint32_t used = kbuff.head - kbuff.tail;
    assertf(used <= KBDBUFFSIZE);

}


// void isr_handler(uint64_t vec, uint64_t err, uint64_t rip) {
  
    
//     if (vec == 14) {
//         uint64_t cr2 = read_cr2();
//         //sfprint("CR2 (fault addr): %8\n", cr2);
//         dump_pf_reason(err);
//     }
//     if (vec == 32) {
//         irq0_handler(vec, err, rip);
//     }
//     if (vec == 33) {
//         //sfprint("Interrupt: %8\n", vec);
//         //sfprint("RIP: %8\n", rip);
//         //sfprint("Error code: %8\n", err);
//         irq1_handler(vec, err, rip);
//     }
//     //for (;;) __asm__ volatile ("hlt");
// }



void remap_pic(void) {
    outb(0x20, 0x11); // init master PIC
    outb(0xA0, 0x11); // init slave PIC
    outb(0x21, 0x20); // master offset = 0x20
    outb(0xA1, 0x28); // slave offset = 0x28
    outb(0x21, 0x04); // master PIC: slave at IRQ2
    outb(0xA1, 0x02); // slave PIC: cascade identity
    outb(0x21, 0x01); // master PIC: 8086 mode
    outb(0xA1, 0x01); // slave PIC: 8086 mode
}

void irq0_handler(isr_frame_t *f) {
    // Send EOI to PIC
    outb(0x20, 0x20);
}

void irq1_handler(isr_frame_t *f) {
    uint8_t scancode = inb(0x60);

    // Optional overrun check (drop newest or oldest)
    uint16_t h = kbuff.head;
    uint16_t t = kbuff.tail;
    if (((h + 1) & 0xFFFF) == t + KBDBUFFSIZE) {
        // buffer full — decide policy: drop this scancode or advance tail
        // kbuff.tail = t + 1; // drop oldest
    }

    kbuff.scancodes[h & KBDBUF_MASK] = scancode;
    kbuff.head = h + 1;

    outb(0x20, 0x20); // EOI
}


// //handle keyboard input
// void irq1_handler(uint64_t vec, uint64_t err, uint64_t rip) {
//     uint8_t scancode = inb(0x60); // read from keyboard data port
//     //sfprint("keyboard input detected. Scan code: %8\n", scancode);
//     kbuff.scancodes[kbuff.head] = scancode;
//     //sfprint("Scan code after store: %8\n", kbuff.scancodes[kbuff.head]);
//     kbuff.head = (kbuff.head + 1) % KBDBUFFSIZE;

//     // decode scancode or buffer it
//     outb(0x20, 0x20); // send EOI to PIC
// }



// enable keyboard interrupts 
void enable_irq(void) {
    uint8_t mask = inb(0x21); // read current Interrupt Mask Register
    mask &= ~(1 << 1); // clear bit 1 to enable IRQ1
    outb(0x21, mask); // update PIC
}


void trigger_ud(void) {
    asm volatile ("ud2"); // guaranteed invalid opcode
}
 
void trigger_pf(void) {
    volatile uint8_t *ptr = (uint8_t*)0x100000000ULL; // 4 GiB
    *ptr = 0xAA; // write to unmapped address
}

void trigger_gp(void) {
    asm volatile ("movw %0, %%ds" :: "r"((uint16_t)0x23) : "memory");
}

void test_exceptions(void) {
    //sfprint("Triggering #UD...\n");
    //trigger_ud();

    sfprint("Triggering #PF...\n");
    trigger_pf();

    sfprint("Triggering #GP...\n");
    trigger_gp();
}