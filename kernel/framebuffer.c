#include <stdarg.h>
#include "framebuffer.h"
#include "serial.h"
#include "multiboot.h"
#include "types.h"
#include "font8x16.h"
#include "mem.h"
#include "serial.h"
#include "string.h"
#include "vga.h"
#include "shell.h"

typedef struct ShellContext ShellContext;

framebuffer_info_t framebuffer = {0}; // zero-init
uint8_t* fbuff_base = 0;// = (uint8_t*)(uintptr_t)fb->framebuffer_addr;

fb_cursor_t fb_cursor = {0, 0}; // start at top-left

typedef struct ShellContext ShellContext;

const struct multiboot_tag_mmap* memb;

#define VGA_COLS  80
#define VGA_ROWS  25
#define VGA_CELLS (VGA_COLS * VGA_ROWS)
#define ATTR      0x07  // light gray on black
#define VGA_MEM   ((volatile uint16_t*)0xB8000)

#define COM1_PORT 0x3F8

// #define FONT_WIDTH 8
// #define FONT_HEIGHT 16

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

void fb_draw_string(const char* str, uint32_t fg, uint32_t bg) {
    // Loop through each character in the input string
    //sfprint("drawing to x, y coord: %8, %8\n", fb_cursor.x, fb_cursor.y);
    for (size_t i = 0; str[i]; ++i) {

        // If the character is a newline, move cursor to start of next line
        if (str[i] == '\n') {
            fb_cursor.x = 0;                  // Reset horizontal position
            fb_cursor.y += FONT_HEIGHT;
            //sfprint("str[%8] y coord: %8\n", i, fb_cursor.y);       // Move down one line
            continue;                         // Skip drawing this character
        }
        // Draw the character at the current cursor position
        fb_draw_char(fbuff_base, framebuffer.pitch,
                     fb_cursor.x, fb_cursor.y,
                     str[i], fg, bg);  
        // Advance the cursor horizontally by one character width
        fb_cursor.x += FONT_WIDTH;
    }
}


void fb_draw_string_with_cursor(const char* str, size_t cursor_pos, uint32_t fg, uint32_t bg, uint32_t cursor_fg, uint32_t cursor_bg) {

    for (size_t i = 0; str[i]; ++i) {
        if (str[i] == '\n') {
            fb_cursor.x = 0;
            fb_cursor.y += FONT_HEIGHT;
            continue;
        }

        // If this is the cursor position, draw with inverted colors
        if (i == cursor_pos) {
            fb_draw_char(fbuff_base, framebuffer.pitch,
                         fb_cursor.x, fb_cursor.y,
                         str[i], cursor_fg, cursor_bg);
        } else {
            fb_draw_char(fbuff_base, framebuffer.pitch,
                         fb_cursor.x, fb_cursor.y,
                         str[i], fg, bg);
        }

        fb_cursor.x += FONT_WIDTH;
    }

    // If cursor is at end of line (after last char), draw a block or underscore
    if (cursor_pos == custom_strlen(str)) {
        fb_draw_char(fbuff_base, framebuffer.pitch,
                     fb_cursor.x, fb_cursor.y,
                     '_', cursor_fg, cursor_bg);
    }
}


void fb_cursor_reset(void) {
    fb_cursor.x = 0;
    fb_cursor.y = 0;
}


char* format_fb_print(const char* fmt, va_list args) {
    char* fbuff = thralloc(256); 
    if (!fbuff) return NULL;

    size_t i = 0;
    const char *p = fmt;

    while (*p && i < 255) {
        if (*p != '%') {
            fbuff[i++] = *p++;
            continue;
        }

        p++; // skip '%'
        if (*p == '\0') break;

        switch (*p) {
            case 'd': {
                int val = va_arg(args, int);
                char buff[32];
                itoa(val, buff);
                for (size_t cnt = 0; buff[cnt] && i < 255; cnt++) {
                    fbuff[i++] = buff[cnt];
                }
                break;
            }
            case '8': {
                uint64_t val = va_arg(args, uint64_t);
                char buff[32];
                llitoa(val, buff);
                for (size_t cnt = 0; buff[cnt] && i < 255; cnt++) {
                    fbuff[i++] = buff[cnt];
                }
                break;
            }
            case 's': {
                const char *sval = va_arg(args, const char*);
                for (size_t cnt = 0; sval[cnt] && i < 255; cnt++) {
                    fbuff[i++] = sval[cnt];
                }
                break;
            }
            case 'x': {
                uint32_t val = va_arg(args, uint32_t);
                char buff[32];
                u32tohex(val, buff);
                for (size_t cnt = 0; buff[cnt] && i < 255; cnt++) {
                    fbuff[i++] = buff[cnt];
                }
                break;
            }
            case 'h': {
                uint8_t val = va_arg(args, int);
                char buff[3];
                u8tohex(val, buff);
                for (size_t cnt = 0; buff[cnt] && i < 255; cnt++) {
                    fbuff[i++] = buff[cnt];
                }
                break;
            }
            case 'c': {
                char val = (char)va_arg(args, int);
                fbuff[i++] = val;
                break;
            }
            case '%': {
                fbuff[i++] = '%';
                break;
            }
            default: {
                fbuff[i++] = '?';
                break;
            }
        }

        p++;
    }

    fbuff[i] = '\0';
    return fbuff;
}

void fbprintf(ShellContext *shell, const char* str, ...) {
    va_list args;
    va_start(args, str);
    char *fbuff = format_fb_print(str, args);
    va_end(args);

    if (fbuff) {
        fb_draw_stringsh(fbuff, custom_strlen(fbuff), FG, BG, shell);
        tfree(fbuff); // free after use
    }
}





void fb_clear(uint32_t bg_color) {
    // Calculate how many bytes each pixel occupies (e.g., 3 for 24bpp, 4 for 32bpp)
    uint32_t bytes_per_pixel = framebuffer.bpp / 8;

    // Number of bytes in a single scanline (row of pixels)
    uint32_t row_bytes = framebuffer.pitch;

    // Total number of bytes in the entire framebuffer (height × pitch)
    uint32_t total_bytes = row_bytes * framebuffer.height;

    // Pointer to the start of the framebuffer memory
    uint8_t* fb = fbuff_base;

    // Loop through every pixel in the framebuffer
    for (uint32_t i = 0; i < total_bytes; i += bytes_per_pixel) {
        // Write the blue component of the background color
        fb[i + 0] = (bg_color >> 0) & 0xFF;

        // Write the green component
        fb[i + 1] = (bg_color >> 8) & 0xFF;

        // Write the red component
        fb[i + 2] = (bg_color >> 16) & 0xFF;

        // Optional: write alpha if using 32bpp (commented out for now)
        // fb[i + 3] = (bg_color >> 24) & 0xFF;
    }
}
//// FORMATTED FRAMBUFFER PRINT ON THE BACKBURNER FOR NOW
// void format_fbprint(const char* fmt, va_list args) {
//     const char *p = fmt;

//     while (*p) {
//         if (*p != '%') {
//             serial_write_char(*p++);
//             continue;
            
//         }

//         p++; // skip '%'

//         if (*p == '\0') break; // stray '%' at end of string

//         switch (*p) {
//             case 'd': { // integer
//                 int val = va_arg(args, int);
//                 char buff[32];
//                 itoa(val,buff);
//                 //int_2_string(val, buff);
//                 fb_draw_string(buff, FG, BG);
//                 break;
//             }
//             case '8': { //prints 64 bit num. still works for all unsigned sizes (8,16,32)
//                 uint64_t val = va_arg(args, uint64_t);
//                 char buff[32];
//                 llitoa(val,buff);
//                 //int_2_string(val, buff);
//                 fb_draw_string(buff, FG, BG);
//                 break;
//             }
//             case 's': { //string
//                 const char *sval = va_arg(args, const char*);
//                 serial_write(sval);
//                 break;
//             }
//             case 'x': {
//                 uint32_t val = va_arg(args, uint32_t);
//                 char buff[32];
//                 u32tohex(val, buff); 
//                 fb_draw_string(buff, FG, BG);
//                 break;
//             }
//             case 'h': { // hex byte
//                 uint8_t val = va_arg(args, int); // promoted to int
//                 char buff[3];
//                 u8tohex(val, buff); // you'll write this
//                 fb_draw_string(buff, FG, BG);
//                 break;
//             }

//             case '%': {
//                 serial_write_char('%'); // handle literal %%
//                 break;
//             }
//             default:
//                 serial_write_char('?'); // unknown format
//                 break;
//         }

//         p++; // move past format specifier
//     }
// }

// void sfbprint(const char* str, ...) {
//     va_list args;
//     va_start(args, str);
//     format_fbprint(str, args);
//     va_end(args);
// }





////////////////////////////////////////////////////////////////////
//quick test for pulling type and size from MB headers/////////////
//////////////////////////////////////////////////////////////////
void walk_mb2(void* mb_ptr) {
    serial_write("Walking MB2 headers\n");
    sfprint("Starting framebuffer addr: %8\n", framebuffer.addr);
    const uint8_t* base = (const uint8_t*)mb_ptr;
    const struct mb2_info* info = (const struct mb2_info*)base;
    const uint8_t* p   = base + 8;
    const uint8_t* end = base + info->total_size;

    uint8_t row = 2;
    int cap = 32; // safety fuse
    int count = 0;

    while (p + sizeof(struct mb2_tag) <= end && cap-- > 0) {
        //serial_write("Entered While loop\n");
        const struct mb2_tag* tag = (const struct mb2_tag*)p;
        if (tag->type == 0 && tag->size == 8) {
            sfprint("Count: %d Framebuffer addr: %8\n", count, framebuffer.addr);
            break;
        }
        //log tags   
        sfprint("Count: %d ", count);
        serial_write("Tag type: "); serial_write_hex32(tag->type);
        serial_write(" Tag size: "); serial_write_hex32(tag->size);
        serial_write("\n");


        // struct multiboot_mmap_entry {
        //     uint64_t base_addr;
        //     uint64_t length;
        //     uint32_t type;
        //     uint32_t reserved;
        // };




        if (tag->type == 6 && tag->size >= sizeof(struct multiboot_tag_mmap)) {
            memb = (const struct multiboot_tag_mmap*)tag;
            init_allocator(memb);   
        }




        if (tag->type == 8 && tag->size >= sizeof(struct mb2_tag_framebuffer)) {
            sfprint("Framebuffer addr: %8\n", framebuffer.addr);
            serial_write("p = 0x");
            serial_write_hex64(0x12345678ABCDEF00ULL);
            serial_write("\n");

            serial_write("Tag type: "); serial_write_hex32(tag->type);
            serial_write(" size: "); serial_write_hex32(tag->size); serial_write("\n");
            const struct mb2_tag_framebuffer* fb = (const struct mb2_tag_framebuffer*)tag;
            size_t fixed_size_without_tag = sizeof(uint64_t) + (5 * sizeof(uint32_t)) + (2 * sizeof(uint8_t));
            void *color_info_addr = (void *)((uint8_t *)fb + sizeof(struct mb2_tag) + fixed_size_without_tag);

            serial_write("Framebuffer info:\n");
            // serial_write("  Addr: 0x"); serial_write_hex64(fb->framebuffer_addr); serial_write("\n");
            // serial_write("  Pitch: "); serial_write_hex32(fb->framebuffer_pitch); serial_write("\n");
            // serial_write("  Width: "); serial_write_hex32(fb->framebuffer_width); serial_write("\n");
            // serial_write("  Height: "); serial_write_hex32(fb->framebuffer_height); serial_write("\n");
            // serial_write("  BPP: "); serial_write_hex8(fb->framebuffer_bpp); serial_write("\n");
            // serial_write("  Type: "); serial_write_hex8(fb->framebuffer_type); serial_write("\n");

            serial_write("  Addr: 0x"); serial_write_hex64(fb->framebuffer_addr); serial_write("\n");
            sfprint("  Pitch: %8\n", fb->framebuffer_pitch);
            sfprint("  Width: %8\n", fb->framebuffer_width);
            sfprint("  Height: %8\n", fb->framebuffer_height);
            sfprint("  BPP: %8\n", fb->framebuffer_bpp); 
            sfprint("  Type: %8\n", fb->framebuffer_type);
            sfprint("expected pitch: %d\n", fb->framebuffer_width * 4);
 
            
            
            // GLOBAL FRAMEBUFFER STRUCT 
            framebuffer.addr   = fb->framebuffer_addr;
            framebuffer.pitch  = fb->framebuffer_pitch;
            framebuffer.width  = fb->framebuffer_width;
            framebuffer.height = fb->framebuffer_height;
            framebuffer.bpp    = fb->framebuffer_bpp;
            framebuffer.type   = fb->framebuffer_type;

            sfprint("fb framebuffer addr: %8\n", fb->framebuffer_addr);
            sfprint("framerbuffer.framebuffer addr: %8\n", framebuffer.addr);
            fbuff_base = (uint8_t*)(uintptr_t)fb->framebuffer_addr;

            
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

            // ALSO UPDATE GLOBAL FRAMEBUFFER STRUCT 
            framebuffer.framebuffer_red_field_position = rgb_info->framebuffer_red_field_position;
            framebuffer.framebuffer_red_mask_size = rgb_info->framebuffer_red_mask_size;
            framebuffer.framebuffer_green_field_position = rgb_info->framebuffer_green_field_position;
            framebuffer.framebuffer_green_mask_size = rgb_info->framebuffer_green_mask_size; 
            framebuffer.framebuffer_blue_field_position = rgb_info->framebuffer_blue_field_position;
            framebuffer.framebuffer_blue_mask_size = rgb_info->framebuffer_blue_mask_size;

            // serial_write("  RFP: "); serial_write_hex8(rgb_info->framebuffer_red_field_position); serial_write("\n");
            // serial_write("  RMS: "); serial_write_hex8(rgb_info->framebuffer_red_mask_size); serial_write("\n");
            // serial_write("  GFP: "); serial_write_hex8(rgb_info->framebuffer_green_field_position); serial_write("\n");
            // serial_write("  GMS: "); serial_write_hex8(rgb_info->framebuffer_green_mask_size); serial_write("\n");
            // serial_write("  BFP: "); serial_write_hex8(rgb_info->framebuffer_blue_field_position); serial_write("\n");  
            // serial_write("  BMS: "); serial_write_hex8(rgb_info->framebuffer_blue_mask_size); serial_write("\n");
            // }
            
            uint8_t* fb_base = (uint8_t*)(uintptr_t)fb->framebuffer_addr;
            sfprint("fb_base addr: %8\n", fb_base);
            }
        }
        // Advance to next tag
        uint64_t adv = align8(tag->size);
        if (adv < sizeof(struct mb2_tag)) break;
        p += adv;
        if (p > end) break;
        ++count;
    
    }
    sfprint("finished MB walk\n");
}