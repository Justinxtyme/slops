//idt.h
#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include "serial.h"

struct idt_entry {
    uint16_t offset_low;     // bits 0–15 of handler address
    uint16_t selector;       // code segment selector
    uint8_t  ist;            // interrupt stack table (usually 0)
    uint8_t  type_attr;      // type and attributes
    uint16_t offset_mid;     // bits 16–31 of handler address
    uint32_t offset_high;    // bits 32–63 of handler address
    uint32_t zero;           // reserved
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern struct idt_entry idt[256];

extern struct idt_ptr idtr;

void zero_idt(void);

int set_idt_entry(int vec, void* handler, uint16_t selector, uint8_t type_attr);

int set_all_idt(void);

void dummy_handler(void* frame);

void isr_handler(uint64_t vec, uint64_t err, uint64_t rip);

// assembly interrupt funcs
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

#endif