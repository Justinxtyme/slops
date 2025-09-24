#include "shell.h"
#include "types.h"
#include "framebuffer.h"
#include "mem.h"
#include "serial.h"
#include "string.h"
#include "kbd.h"
#include "types.h"
#include "fat.h"
#include "parser.h"
#include "assertf.h"
#include <stddef.h>

#define PROMPT_LEN 8


char linebuff[LINEBUFF_SIZE];
int line_len = 0;
int max_lines;
int max_chars;
int cursor_pos = 0;



void fb_draw_stringsh(const char* str, int len, uint32_t fg, uint32_t bg, struct ShellContext *shell) {
    for (size_t i = 0; i < len; ++i) {
        char c = str[i];

        if (c == '\n') {
            scroll_on_newline(shell);
            continue;
        }

        fb_draw_char(fbuff_base, framebuffer.pitch,
                     fb_cursor.x, fb_cursor.y,
                     c, fg, bg);

        fb_cursor.x += FONT_WIDTH;

        // Wrap if line exceeds screen width
        if (fb_cursor.x >= framebuffer.width) {
            scroll_on_wrap(shell);
        }
    }
}

void scroll_on_newline(ShellContext *shell) {
    shell->shell_line++;
    clamp_n_scroll(shell);
    fb_cursor.x = 0;
    fb_cursor.y = shell->shell_line * FONT_HEIGHT;
    clamp_n_scroll(shell);
}

void scroll_on_wrap(ShellContext *shell) {
    sfprint("\n\n\nWrapping\n");
    fb_cursor.x = 0;
    shell->shell_line++;
    fb_cursor.y = shell->shell_line * FONT_HEIGHT;
    clamp_n_scroll(shell);
}


// void scroll_history_up(ShellContext *shell) {
//     // Only scroll if there are more lines above the current view
//     if (shell->history_count > max_lines && shell->scroll_offset < (shell->history_count - max_lines)) {
//         shell->scroll_offset++;
//         redraw_from_history(shell);
//     }
// }

// void scroll_history_down(ShellContext *shell) {
//     // Only scroll if we’re not already at the bottom
//     if (shell->scroll_offset > 0) {
//         shell->scroll_offset--;
//         redraw_from_history(shell);
//     }
// }

// void redraw_from_history(ShellContext *shell) {
//     fb_clear(BG);
//     int start = shell->history_count - max_lines - shell->scroll_offset;
//     if (start < 0) start = 0;

//     for (int i = 0; i < max_lines && (start + i) < shell->history_count; i++) {
//         fb_draw_string(shell->line_history[start + i], FG, BG);
//     }
// }




void init_shell_lines(ShellContext *shell) {
    sfprint("Initialzing shell vars\n");
    shell->shell_line = 0;
    max_lines = framebuffer.height / (FONT_HEIGHT) - 1;
    max_chars = framebuffer.width / (FONT_WIDTH) - 1;
    sfprint("\n\n\n\nmax chars: %d\n", max_chars);
    shell->scroll_offset = 0;
    sfprint("scroll offset: %d\n", shell->scroll_offset);
    shell->history_count = 0;
    sfprint("History count: %d\n", shell->history_count);
    shell->line_history = cralloc(MAX_HISTORY_LINES, sizeof(char*));
    assertf(shell->line_history != NULL);

    for (int i = 0; i < MAX_HISTORY_LINES; ++i) {
        shell->line_history[i] = cralloc(max_chars + 1, sizeof(char)); // +1 for null terminator
        sfprint("line_history[%d] = %d\n", i, shell->line_history[i]);
        assertf(shell->line_history[i] != NULL);
    }


    sfprint("Shell vars initialized\n");
}

void draw_prompt(void) {
    fb_draw_string("THRASH: ", 0x0099FFFF, BG);
}

//void translate_scancode
void shell_cursor_reset(ShellContext *shell) {
    fb_cursor.x = 0;
    fb_cursor.y = shell->shell_line * FONT_HEIGHT;
    //sfprint("cursor reset: %8, %8\n", fb_cursor.x, fb_cursor.y);
    draw_prompt();
}



void clear_line(ShellContext *shell) {
    shell_cursor_reset(shell);
    char spaces[256];
    int max_chars = sizeof(spaces) - 1;
    int count = max_chars - 1;
    for (int i = 0; i < count; i++) spaces[i] = ' ';
    spaces[count] = '\0';

    fb_draw_string(spaces, FG, BG);
    fb_cursor.x = 0;
    //cursor_pos = 0;
    draw_prompt();
}
void clear_line_no_prompt(ShellContext *shell) {
    shell_cursor_reset(shell);
    char spaces[256];
    int max_chars = sizeof(spaces) - 1;
    int count = max_chars - 1;
    for (int i = 0; i < count; i++) spaces[i] = ' ';
    spaces[count] = '\0';

    fb_draw_string(spaces, FG, BG);
    fb_cursor.x = 0;
    //cursor_pos = 0;
    //draw_prompt();
}



void scroll_screen_up(ShellContext *shell) {
    //sfprint("shell_line: %d\n", shell->shell_line);
    //sfprint("max_lines: %d\n", max_lines);
    //sfprint("cursor.y: %d\n", fb_cursor.y);
    if (shell->shell_line >= max_lines) {
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
        shell->shell_line = max_lines - 1;
    }
}

void clamp_n_scroll(ShellContext *shell) {
    if (shell->shell_line >= max_lines) {
        sfprint("SCROLL triggered at shell_line=%d\n", shell->shell_line);
        scroll_screen_up(shell);
        return;
    }
    return;
}


int process_scancode(ShellContext *shell, uint8_t scancode) {
    //sfprint("Processing: %8\n", scancode);
    if (scancode == 75 && cursor_pos > 0) {
        cursor_pos--;
        clear_line(shell);
        fb_draw_string_with_cursor(linebuff, cursor_pos, FG, BG, BG, FG);
        return 0;
    }
    if (scancode == 77 && cursor_pos < line_len) {
        cursor_pos++;
        clear_line(shell);
        fb_draw_string_with_cursor(linebuff, cursor_pos, FG, BG, BG, FG);
        return 0;
    }
    uint8_t ascii = scancode2ascii(scancode);
    //sfprint("ASCII: %8\n", ascii);
    //sfprint("Line length: %8\n", line_len);

    if (ascii == '\n') {
        linebuff[line_len] = '\0';
        // clear line and reprint without cursor
        clamp_n_scroll(shell);
        clear_line(shell);
        fb_draw_string(linebuff, FG, BG);
        // Advance to next line
        shell->shell_line++;
        clamp_n_scroll(shell);
        line_len = 0;
        cursor_pos = 0;
        process_cmd(shell, linebuff);
        //process_input_segments(shell, linebuff);
        return 0;

    } else if (ascii == '\b' && line_len > 0) {
        if (cursor_pos > 0) {
            shift_left(linebuff, cursor_pos - 1, line_len);
            cursor_pos--;
            line_len--;
            linebuff[line_len] = '\0';
        }
        clear_line(shell);
        fb_draw_string_with_cursor(linebuff, cursor_pos, FG, BG, BG, FG);
        //fb_draw_string(linebuff, FG, BG);
        return 0;

    } else if (isprint(ascii) && line_len  < (max_chars - 1)) {
        sfprint("line length: %8   max_chars: %8  cursor pos: %8\n", line_len, max_chars, cursor_pos);
        //sfprint("shell line: %8\n", shell_line);
        //sfprint("ISPRINT drawing to x, y coord: %8, %8\n", fb_cursor.x, fb_cursor.y);
        if (line_len + PROMPT_LEN >= max_chars) {
            sfprint("wrapping\n");
            fb_cursor.x = 0;
            fb_cursor.y += FONT_HEIGHT;
        }

        if (cursor_pos < line_len) {
            shift_right(linebuff, cursor_pos, line_len);
        }
        linebuff[cursor_pos] = ascii;
        cursor_pos++;
        //line_len++;
        linebuff[++line_len] = '\0';
        assertf(linebuff[line_len] == '\0');

        //sfprint("coord right before clear_line: %8, %8\n", fb_cursor.x, fb_cursor.y);
        clear_line(shell);
        //sfprint("coord right before redraw: %8, %8\n", fb_cursor.x, fb_cursor.y);
        fb_draw_string_with_cursor(linebuff, cursor_pos, FG, BG, BG, FG);
        //fb_draw_string(linebuff, FG, BG);
        return 0;

    } else if (ascii == 255) {
        //sfprint("255 returned\n");

        return 0;

    } else {
        //sfprint("Error processing scan code");
        return 1;
    }
}

int process_cmd(ShellContext *shell, char *cmd) {
    sfprint("COMMAND PROCESSING: %s\n", cmd);
    clear_line_no_prompt(shell);
    shell->shell_line++;
    process_input_segments(shell, linebuff);
    return 0;
}


void shift_right(char* buf, size_t pos, size_t len) {
    for (size_t i = len; i > pos; i--) {
        buf[i] = buf[i - 1];
    }
}
void shift_left(char* buf, size_t pos, size_t len) {
    for (size_t i = pos; i < len - 1; i++) {
        buf[i] = buf[i + 1];
    }
    buf[len - 1] = '\0';
}