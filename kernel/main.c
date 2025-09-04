// kernel/main.c
#include <stdint.h>
#include "multiboot.h"

static inline void outb(uint16_t p, uint8_t v){
    __asm__ __volatile__("outb %0,%1"::"a"(v),"Nd"(p));
}
static inline void cpu_halt(void){
    for(;;){ __asm__ __volatile__("cli; hlt"); }
}


#define VGA_CELLS  (VGA_COLS * VGA_ROWS)


#define VGA_MEM   ((volatile uint16_t*)0xB8000)
#define VGA_COLS  80
#define VGA_ROWS  25
#define ATTR      0x07  // light gray on black

static inline void vga_clear(uint8_t attr) {
    uint16_t fill = ((uint16_t)attr << 8) | ' ';
    for (uint16_t i = 0; i < VGA_CELLS; i++) {
        VGA_MEM[i] = fill;
    }
}

static inline uint32_t align8(uint32_t x) {
    return (x + 7u) & ~7u;
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

void walk_mb2(uint32_t mb_ptr) {
    const uint8_t* base = (const uint8_t*)mb_ptr;
    const struct mb2_info* info = (const struct mb2_info*)base;
    const uint8_t* p   = base + 8;
    const uint8_t* end = base + info->total_size;

    uint8_t row = 2;
    int cap = 32; // safety fuse

    while (p + sizeof(struct mb2_tag) <= end && cap-- > 0) {
        const struct mb2_tag* tag = (const struct mb2_tag*)p;
        if (tag->type == 0 && tag->size == 8) break;

        // Print type
        vga_putc(row, 0, 'T');
        vga_putc(row, 1, ':');
        vga_print_hex32(row, 3, tag->type);

        // Print size
        vga_putc(row, 12, 'S');
        vga_putc(row, 13, ':');
        vga_print_hex32(row, 15, tag->size);

        row++;
        if (row >= VGA_ROWS) row = VGA_ROWS - 1; // pin to bottom

        // Advance to next tag
        uint32_t adv = align8(tag->size);
        if (adv < sizeof(struct mb2_tag)) break;
        p += adv;
        if (p > end) break;
    }
}

void kernel_main(uint32_t mb_info) {
    outb(0xE9, 'M'); // debug marker
    vga_clear(ATTR); // your clear function from before
    walk_mb2(mb_info);
    for (;;) { __asm__ __volatile__("cli; hlt"); }
}






// void kernel_main(uint32_t mb_info) {
//     (void)mb_info;
//     outb(0xE9, 'M');

//     volatile uint16_t* vga = (uint16_t*)0xB8000;
//     vga_clear(0x07); // clear to gray-on-black
//     //const char *msg = "Hello";
//     //for (int i = 0; msg[i]; i++) {
//     //    vga[80 + i] = (0x07 << 8) | msg[i]; // row 1
//     //}
    
//     cpu_halt();
// }
