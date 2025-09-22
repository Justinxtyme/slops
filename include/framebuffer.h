//framebuffer.h
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stdarg.h>
#include "types.h"

#define FONT_WIDTH 8
#define FONT_HEIGHT 16
#define FG 0x00FFFFFF
#define BG 0x00000000

typedef struct ShellContext ShellContext;

typedef struct {
    uint32_t x; // pixel position
    uint32_t y;
} fb_cursor_t;


typedef struct {
    uint64_t addr;     // physical framebuffer address
    uint32_t pitch;    // bytes per scanline
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;      // bits per pixel
    uint8_t  type;
    uint8_t framebuffer_red_field_position;
    uint8_t framebuffer_red_mask_size;
    uint8_t framebuffer_green_field_position;
    uint8_t framebuffer_green_mask_size;
    uint8_t framebuffer_blue_field_position;
    uint8_t framebuffer_blue_mask_size;
} framebuffer_info_t;


extern fb_cursor_t fb_cursor;
extern framebuffer_info_t framebuffer;
extern uint8_t* fbuff_base;



void walk_mb2(void* mb_ptr);

void fb_draw_char(uint8_t* fb, uint32_t pitch,
                  uint32_t x, uint32_t y,
                  char c, uint32_t fg, uint32_t bg);


void fb_draw_string(const char* str, uint32_t fg, uint32_t bg);

void fb_draw_string_with_cursor(const char* str, size_t cursor_pos, uint32_t fg,
                                 uint32_t bg, uint32_t cursor_fg, uint32_t cursor_bg);

void fb_cursor_reset(void);

void fb_clear(uint32_t bg_color);


void fbprintf(const char* str, ...);

char* format_fb_print(const char* fmt, va_list args);

void fb_draw_stringsh(const char* str, int len, uint32_t fg, uint32_t bg, struct ShellContext *shell);

// On the backburner for now
// void sfbprint(const char* str, ...);

// void format_fbprint(const char* fmt, va_list args);

#endif