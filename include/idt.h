//idt.h
#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include "serial.h"
#include "shell.h"

struct idt_entry {
    uint16_t offset_low;     // bits 0–15 of handler address
    uint16_t selector;       // code segment selector
    uint8_t  ist;            // interrupt stack table (usually 0)
    uint8_t  type_attr;      // type and attributes
    uint16_t offset_mid;     // bits 16–31 of handler address
    uint32_t offset_high;    // bits 32–63 of handler address
    uint32_t zero;           // reserved
} __attribute__((packed));

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
    uint64_t vec;       // our push
    uint64_t err;       // CPU or 0
    uint64_t rip;       // CPU
    uint64_t cs;        // CPU
    uint64_t rflags;    // CPU
    uint64_t rsp;       // CPU if CPL change
    uint64_t ss;        // CPU if CPL change
} isr_frame_t;


struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern struct idt_entry idt[256];

extern struct idt_ptr idtr;

int read_sc(ShellContext *shell);


void zero_idt(void);

int set_idt_entry(int vec, void* handler, uint16_t selector, uint8_t type_attr);

int set_all_idt(void);

void isr_handler(isr_frame_t *f);

void remap_pic(void);

void enable_irq(void);

void irq0_handler(isr_frame_t *f);

void irq1_handler(isr_frame_t *f);

void trigger_ud(void);
 
void trigger_pf(void);

void trigger_gp(void);

void test_exceptions(void);


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
extern void isr32();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
#endif