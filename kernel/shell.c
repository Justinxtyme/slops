#include "shell.h"
#include "types.h"
#include "framebuffer.h"
#include "mem.h"
#include "serial.h"
#include "string.h"
#include "kbd.h"
//#include <ctype.h>
#include <stddef.h> 

char linebuff[LINEBUFF_SIZE];
size_t line_len = 0;
int shell_line = 0;
int max_lines;
int max_chars;

void init_shell_lines(void) {
    shell_line = 0;
    max_lines = framebuffer.height / (FONT_HEIGHT) - 1;
    max_chars = framebuffer.width / FONT_WIDTH;
    sfprint("shell line: %8\n", shell_line);
}

//void translate_scancode 
void shell_cursor_reset(void) {
    fb_cursor.x = 0;
    fb_cursor.y = shell_line * FONT_HEIGHT;
    sfprint("cursor reset: %8, %8\n", fb_cursor.x, fb_cursor.y); 
}



void clear_line() {
    shell_cursor_reset();
    char spaces[256];
    int max_chars = sizeof(spaces) - 1;
    int count = max_chars - 1;
    for (int i = 0; i < count; i++) spaces[i] = ' ';
    spaces[count] = '\0';

    fb_draw_string(spaces, FG, BG);
    fb_cursor.x = 0;
}




void scroll_screen_up() {
    sfprint("shell_line: %d\n", shell_line);
    sfprint("max_lines: %d\n", max_lines);
    sfprint("cursor.y: %d\n", fb_cursor.y);
    if (shell_line >= max_lines) {
        uint32_t* fb = (uint32_t*)(uintptr_t)framebuffer.addr;
        int pixels_per_row = framebuffer.pitch / 4;
        int total_rows = framebuffer.height;

        // Scroll each pixel row up by one
        for (int row = 0; row < total_rows - FONT_HEIGHT; row++) {
            uint32_t* dest = fb + row * pixels_per_row;
            uint32_t* src  = fb + (row + FONT_HEIGHT) * pixels_per_row;
            for (int x = 0; x < pixels_per_row; x++) {
                dest[x] = src[x];
            }
        }

        // Clear bottom FONT_HEIGHT rows
        for (int row = total_rows - FONT_HEIGHT; row < total_rows; row++) {
            uint32_t* dest = fb + row * pixels_per_row;
            for (int x = 0; x < pixels_per_row; x++) {
                dest[x] = 0x00000000;
            }
        }
        shell_line = max_lines - 1;
    }
}



int process_scancode(uint8_t scancode) {
    sfprint("Processing: %8\n", scancode);
    uint8_t ascii = scancode2ascii(scancode);
    sfprint("ASCII: %8\n", ascii);
    sfprint("Line length: %8\n", line_len);

    if (ascii == '\n') {
        sfprint("Newline detected, x,y = %8, %8\n", fb_cursor.x, fb_cursor.y);
        linebuff[line_len] = '\0';

        // Print the command on the current line
        clear_line();
        fb_draw_string(linebuff, FG, BG);

        // Advance to next line
        shell_line++;
        if (shell_line >= max_lines) {
            scroll_screen_up();
            shell_line = max_lines - 1;
        }

        line_len = 0;
        return 0;

    } else if (ascii == '\b' && line_len > 0) {
        line_len--;
        linebuff[line_len] = '\0';

        clear_line();
        fb_draw_string(linebuff, FG, BG);
        return 0;

    } else if (isprint(ascii) && line_len < LINEBUFF_SIZE - 1) {
        sfprint("shell line: %8\n", shell_line);
        sfprint("ISPRINT drawing to x, y coord: %8, %8\n", fb_cursor.x, fb_cursor.y);
        if (line_len >= max_chars) {
            fb_cursor.x = 0;
            fb_cursor.y += FONT_HEIGHT; 
        }

        linebuff[line_len++] = ascii;
        linebuff[line_len] = '\0';
        sfprint("coord right before clear_line: %8, %8\n", fb_cursor.x, fb_cursor.y);
        clear_line();
        sfprint("coord right before redraw: %8, %8\n", fb_cursor.x, fb_cursor.y);
        fb_draw_string(linebuff, FG, BG);
        return 0;

    } else {
        sfprint("Error processing scan code");
        return 1;
    }
}


int process_cmd(char *cmd) {
    shell_cursor_reset();

    fb_draw_string(cmd, FG, BG);

    shell_line++;
    if (shell_line >= max_lines) {
        scroll_screen_up();
        shell_line = max_lines - 1;
    }

    return 0;
}

// int process_scancode(uint8_t scancode) {
//     sfprint("Processing: %8\n", scancode);
//     uint8_t ascii = scancode2ascii(scancode);
//     sfprint("ASCII: %8\n", ascii);
//     sfprint("Line length: %8\n", line_len);
//     if (ascii == '\n') {
//         linebuff[line_len] = '\0';
//         //scroll_screen_up();
//         clear_line();
//         process_cmd(linebuff);
//         line_len = 0;;
//         return 0;
    
//     } else if (ascii == '\b' && line_len > 0) {
//         line_len--;
//         linebuff[line_len] = '\0';
//         if (fb_cursor.x >= FONT_WIDTH) fb_cursor.x -= FONT_WIDTH; //move cursor back one space
//         clear_line(); 
//         fb_draw_string(linebuff, FG, BG); //redraw line
//         return 0;    
    
//     } else if (isprint(ascii) && line_len < LINEBUFF_SIZE - 1) {
//         int max_chars = framebuffer.width / FONT_WIDTH;
//         if (line_len >= max_chars) {
//             //scroll_screen_up();
//             //shell_cursor_reset();
//         }
//         linebuff[line_len++] = ascii;
//         linebuff[line_len] = '\0';
//         fb_cursor.x = 0;
//         if (shell_line >= max_lines) {
//             scroll_screen_up();
//             shell_line = max_lines - 1;
//         }

//         fb_cursor.y = shell_line * FONT_HEIGHT;

//         clear_line(); 
//         fb_draw_string(linebuff, FG, BG);
//         return 0; 
    
//     } else {
//         sfprint("Error processing scan code");
//         // do something?? idk
//         return 1;     
//     }
// }