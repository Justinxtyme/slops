#include "framebuffer.h"
#include "serial.h"
#include "multiboot.h"
#include "types.h"
#include "font8x16.h"

framebuffer_info_t framebuffer = {0}; // zero-init
uint8_t* fbuff_base = 0;// = (uint8_t*)(uintptr_t)fb->framebuffer_addr;

fb_cursor_t fb_cursor = {0, 0}; // start at top-left

#define VGA_COLS  80
#define VGA_ROWS  25
#define VGA_CELLS (VGA_COLS * VGA_ROWS)
#define ATTR      0x07  // light gray on black
#define VGA_MEM   ((volatile uint16_t*)0xB8000)

#define COM1_PORT 0x3F8

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

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
        //uint8_t bits = firacode[(uint8_t)c][row];
        for (int col = 0; col < 8; col++) {
            uint32_t color = (bits & (0x80 >> col)) ? fg : bg;
            fb_putpixel(fb, pitch, x + col, y + row, color);
        }
    }
}
// void fb_draw_string(uint8_t* fb, uint32_t pitch,
//                     uint32_t x, uint32_t y,
//                     const char* s, uint32_t fg, uint32_t bg) {
//     while (*s) {
//         fb_draw_char(fb, pitch, x, y, *s++, fg, bg);
//         x += 8; // advance by char width
//     }
// }

void fb_draw_string(const char* str, uint32_t fg, uint32_t bg) {
    // Loop through each character in the input string
    for (size_t i = 0; str[i]; ++i) {

        // If the character is a newline, move cursor to start of next line
        if (str[i] == '\n') {
            fb_cursor.x = 0;                  // Reset horizontal position
            fb_cursor.y += FONT_HEIGHT;       // Move down one line
            continue;                         // Skip drawing this character
        }

        // Draw the character at the current cursor position
        fb_draw_char(fbuff_base, framebuffer.pitch,
                     fb_cursor.x, fb_cursor.y,
                     str[i], fg, bg);

        // Advance the cursor horizontally by one character width
        fb_cursor.x += FONT_WIDTH;

        // If cursor reaches the end of the screen width, wrap to next line
        if (fb_cursor.x + FONT_WIDTH > framebuffer.width) {
            fb_cursor.x = 0;                  // Reset horizontal position
            fb_cursor.y += FONT_HEIGHT;       // Move down one line
        }

        // If cursor reaches the bottom of the screen, wrap to top (no scroll yet)
        if (fb_cursor.y + FONT_HEIGHT > framebuffer.height) {
            fb_cursor.y = 0;                  // Reset vertical position
            // You could implement scrolling here later
        }
    }
}

void fb_cursor_reset(void) {
    fb_cursor.x = 0;
    fb_cursor.y = 0;
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
        serial_write("Entered While loop\n");
        const struct mb2_tag* tag = (const struct mb2_tag*)p;
        if (tag->type == 0 && tag->size == 8) {
            sfprint("Framebuffer addr: %8\n", framebuffer.addr);
            break;
        }
        //log tags   
        sfprint("Count: %d ", count);
        serial_write("Tag type: "); serial_write_hex32(tag->type);
        serial_write(" Tag size: "); serial_write_hex32(tag->size);
        serial_write("\n");

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
            serial_write("  Addr: 0x"); serial_write_hex64(fb->framebuffer_addr); serial_write("\n");
            serial_write("  Pitch: "); serial_write_hex32(fb->framebuffer_pitch); serial_write("\n");
            serial_write("  Width: "); serial_write_hex32(fb->framebuffer_width); serial_write("\n");
            serial_write("  Height: "); serial_write_hex32(fb->framebuffer_height); serial_write("\n");
            serial_write("  BPP: "); serial_write_hex8(fb->framebuffer_bpp); serial_write("\n");
            serial_write("  Type: "); serial_write_hex8(fb->framebuffer_type); serial_write("\n");
            
            
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
            fb_draw_string("Hello framebuffer.... and world!", 0x00FFFFFF, 0x00000000);
            }

        }
        // Advance to next tag
        uint64_t adv = align8(tag->size);
        if (adv < sizeof(struct mb2_tag)) break;
        p += adv;
        if (p > end) break;
        ++count;
    
    }
}