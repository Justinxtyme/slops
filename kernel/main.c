// kernel/main.c
#include <stdint.h>
#include "multiboot.h"
#include "gdt.h"
#include "serial.h"
#include "font8x16.h"

#define VGA_COLS  80
#define VGA_ROWS  25
#define VGA_CELLS (VGA_COLS * VGA_ROWS)
#define ATTR      0x07  // light gray on black
#define VGA_MEM   ((volatile uint16_t*)0xB8000)

#define COM1_PORT 0x3F8

typedef unsigned long size_t;


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


static inline void fb_putpixel(uint8_t* fb, uint32_t pitch,
                               uint32_t x, uint32_t y, uint32_t color) {
    *(uint32_t*)(fb + y * pitch + x * 4) = color;
}

void fb_draw_char(uint8_t* fb, uint32_t pitch,
                  uint32_t x, uint32_t y,
                  char c, uint32_t fg, uint32_t bg) {
    for (int row = 0; row < 16; row++) {
        uint8_t bits = font8x16[(uint8_t)c][row];
        for (int col = 0; col < 8; col++) {
            uint32_t color = (bits & (0x80 >> col)) ? fg : bg;
            fb_putpixel(fb, pitch, x + col, y + row, color);
        }
    }
}
void fb_draw_string(uint8_t* fb, uint32_t pitch,
                    uint32_t x, uint32_t y,
                    const char* s, uint32_t fg, uint32_t bg) {
    while (*s) {
        fb_draw_char(fb, pitch, x, y, *s++, fg, bg);
        x += 8; // advance by char width
    }
}




////////////////////////////////////////////////////////////////////
//quick test for pulling type and size from MB headers/////////////
//////////////////////////////////////////////////////////////////
void walk_mb2(void* mb_ptr) {
    serial_write("Entered walk_mb2\n");
    const uint8_t* base = (const uint8_t*)mb_ptr;
    const struct mb2_info* info = (const struct mb2_info*)base;
    const uint8_t* p   = base + 8;
    const uint8_t* end = base + info->total_size;

    uint8_t row = 2;
    int cap = 32; // safety fuse

    while (p + sizeof(struct mb2_tag) <= end && cap-- > 0) {
        serial_write("Entered While loop\n");
        const struct mb2_tag* tag = (const struct mb2_tag*)p;
        if (tag->type == 0 && tag->size == 8) break;

        if (tag->type == 8 && tag->size >= sizeof(struct mb2_tag_framebuffer)) {
            serial_write("p = 0x");
            serial_write_hex64(0x12345678ABCDEF00ULL);
            serial_write("\n");

            serial_write("Tag type: "); serial_write_hex32(tag->type);
            serial_write(" size: "); serial_write_hex32(tag->size); serial_write("\n");
            const struct mb2_tag_framebuffer* fb = (const struct mb2_tag_framebuffer*)tag;
            size_t fixed_size_without_tag = sizeof(uint64_t) + (5 * sizeof(uint32_t)) + (2 * sizeof(uint8_t));
            void *color_info_addr = (void *)((uint8_t *)fb + sizeof(struct mb2_tag) + fixed_size_without_tag);

            serial_write("Framebuffer info:\n");
            serial_write("  Addr: 0x"); serial_write_hex64(fb->framebuffer_addr); serial_write("\n");
            serial_write("  Pitch: "); serial_write_hex32(fb->framebuffer_pitch); serial_write("\n");
            serial_write("  Width: "); serial_write_hex32(fb->framebuffer_width); serial_write("\n");
            serial_write("  Height: "); serial_write_hex32(fb->framebuffer_height); serial_write("\n");
            serial_write("  BPP: "); serial_write_hex8(fb->framebuffer_bpp); serial_write("\n");
            serial_write("  Type: "); serial_write_hex8(fb->framebuffer_type); serial_write("\n");
            if (fb->framebuffer_type == 1) {
            // PULL RGB DATA AND PRINT 
            struct mb2_color_info_rgb *rgb_info = (struct mb2_color_info_rgb *)color_info_addr;
            // You can now access fields using dot notation
            uint8_t red_pos = rgb_info->framebuffer_red_field_position;
            uint8_t red_mask_size = rgb_info->framebuffer_red_mask_size;
            uint8_t green_pos = rgb_info->framebuffer_green_field_position;
            uint8_t green_mask_size = rgb_info->framebuffer_green_mask_size; 
            uint8_t blue_pos = rgb_info->framebuffer_blue_field_position;
            uint8_t blue_mask_size = rgb_info->framebuffer_blue_mask_size;   
            serial_write("  RFP: "); serial_write_hex8(rgb_info->framebuffer_red_field_position); serial_write("\n");
            serial_write("  RMS: "); serial_write_hex8(rgb_info->framebuffer_red_mask_size); serial_write("\n");
            serial_write("  GFP: "); serial_write_hex8(rgb_info->framebuffer_green_field_position); serial_write("\n");
            serial_write("  GMS: "); serial_write_hex8(rgb_info->framebuffer_green_mask_size); serial_write("\n");
            serial_write("  BFP: "); serial_write_hex8(rgb_info->framebuffer_blue_field_position); serial_write("\n");  
            serial_write("  BMS: "); serial_write_hex8(rgb_info->framebuffer_blue_mask_size); serial_write("\n");
            }
            
            uint8_t* fb_base = (uint8_t*)(uintptr_t)fb->framebuffer_addr;
            fb_draw_string(fb_base, fb->framebuffer_pitch, 10, 10,
                        "Hello framebuffer.... and world!", 0x00FFFFFF, 0x00000000);
        }
       

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


