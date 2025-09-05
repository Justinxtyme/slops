// kernel/main.c
#include <stdint.h>
#include "multiboot.h"
#include "gdt.h"


#define VGA_COLS  80
#define VGA_ROWS  25
#define VGA_CELLS (VGA_COLS * VGA_ROWS)
#define ATTR      0x07  // light gray on black
#define VGA_MEM   ((volatile uint16_t*)0xB8000)

#define COM1_PORT 0x3F8



static inline void cpu_halt(void) {
    for(;;){ __asm__ __volatile__("cli; hlt"); }
}



///////////////////////////////////////////////////////////////
// VGA helpers for printing to screen/////////////////////////
/////////////////////////////////////////////////////////////
static inline void vga_clear(uint8_t attr) {
    uint16_t fill = ((uint16_t)attr << 8) | ' ';
    for (uint16_t i = 0; i < VGA_CELLS; i++) {
        VGA_MEM[i] = fill;
    }
}

static inline uint64_t align8(uint64_t x) {
    return (x + 7ull) & ~7ull;
}

static void vga_putc(uint8_t row, uint8_t col, char ch) {
    VGA_MEM[row * VGA_COLS + col] = ((uint16_t)ATTR << 8) | (uint8_t)ch;
}

static void vga_print_hex32(uint8_t row, uint8_t col, uint32_t val) {
    for (int i = 0; i < 8; i++) {
        uint8_t nib = (val >> ((7 - i) * 4)) & 0xF;
        char c = (nib < 10) ? ('0' + nib) : ('A' + (nib - 10));
        vga_putc(row, col + i, c);
    }
}


//////////////////////////////////////////////////////////////////
//SERIAL OUTPUT TO LINUX ////////////////////////////////////////
////////////////////////////////////////////////////////////////   
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void serial_init() {
    outb(COM1_PORT + 1, 0x00);    // Disable interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB
    outb(COM1_PORT + 0, 0x03);    // Set baud rate divisor (low byte)
    outb(COM1_PORT + 1, 0x00);    // High byte
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static void serial_write_char(char c) {
    while (!(inb(COM1_PORT + 5) & 0x20)); // Wait until transmit buffer is empty
    outb(COM1_PORT, c);
}

static void serial_write(const char* s) {
    while (*s) {
        serial_write_char(*s++);
    }
}


////////////////////////////////////////////////////////////////////
//quick test for pulling type and size from MB headers/////////////
//////////////////////////////////////////////////////////////////
void walk_mb2(void* mb_ptr) {
    const uint8_t* base = (const uint8_t*)mb_ptr;
    const struct mb2_info* info = (const struct mb2_info*)base;
    const uint8_t* p   = base + 8;
    const uint8_t* end = base + info->total_size;

    uint8_t row = 2;
    int cap = 32; // safety fuse

    while (p + sizeof(struct mb2_tag) <= end && cap-- > 0) {
        const struct mb2_tag* tag = (const struct mb2_tag*)p;
        if (tag->type == 0 && tag->size == 8) break;

        int col = 0;
        // Print type
        vga_putc(row, 0, 'T');
        vga_putc(row, 1, 'Y');
        vga_putc(row, 2, 'P');
        vga_putc(row, 3, 'E');
        vga_putc(row, 4, ':');
        vga_print_hex32(row, 6, tag->type);

        // Print size
        vga_putc(row, 15, 'S');
        vga_putc(row, 16, 'I');
        vga_putc(row, 17, 'Z');
        vga_putc(row, 18, 'E');
        vga_putc(row, 19, ':');
        vga_print_hex32(row, 21, tag->size);

        row++;
        if (row >= VGA_ROWS) row = VGA_ROWS - 1; // pin to bottom

        // Advance to next tag
        uint64_t adv = align8(tag->size);
        if (adv < sizeof(struct mb2_tag)) break;
        p += adv;
        if (p > end) break;
    }
}



///////////////////////////////////////////////////////////////
// quick test for proper GDT setup////////////////////////////
/////////////////////////////////////////////////////////////
static void log_gdt_state(void) {
    uint16_t cs, ds, es, ss;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile ("mov %%ds, %0" : "=r"(ds));
    __asm__ volatile ("mov %%es, %0" : "=r"(es));
    __asm__ volatile ("mov %%ss, %0" : "=r"(ss));

    serial_write("CS: "); serial_write_char('0' + ((cs >> 4) & 0xF)); serial_write_char('0' + (cs & 0xF)); serial_write("\n");
    serial_write("DS: "); serial_write_char('0' + ((ds >> 4) & 0xF)); serial_write_char('0' + (ds & 0xF)); serial_write("\n");
    serial_write("ES: "); serial_write_char('0' + ((es >> 4) & 0xF)); serial_write_char('0' + (es & 0xF)); serial_write("\n");
    serial_write("SS: "); serial_write_char('0' + ((ss >> 4) & 0xF)); serial_write_char('0' + (ss & 0xF)); serial_write("\n");

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) gdtr;

    __asm__ volatile ("sgdt %0" : "=m"(gdtr));

    serial_write("GDTR.limit: "); serial_write_char('0' + ((gdtr.limit >> 4) & 0xF)); serial_write_char('0' + (gdtr.limit & 0xF)); serial_write("\n");
    serial_write("GDTR.base: ");  // Just dump low byte for sanity
    serial_write_char('0' + ((gdtr.base >> 4) & 0xF)); serial_write_char('0' + (gdtr.base & 0xF)); serial_write("\n");
}



///////////////////////////////////////////////////////////////
//ENTRY POINT FROM BOOTLOAD///////////////////////////////////
/////////////////////////////////////////////////////////////
void kernel_main(void* mb_info) {
    outb(0xE9, 'M'); // debug marker
    gdt_init();
    gdt_install();
    vga_clear(ATTR);
    serial_init();
    serial_write("Hello from kernel_main!\n");
    log_gdt_state();

    vga_clear(ATTR);
    walk_mb2(mb_info);
    //cpu_halt();
}


