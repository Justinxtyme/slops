#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include "kbd.h"
#include "types.h"

#define INPUT_SIZE 1024
#define LINEBUFF_SIZE 128
#define MAX_HISTORY_LINES 128

typedef struct ShellContext {
    char input[INPUT_SIZE];   // User input buffer
    _Bool running;              // Shell loop control flag
    int shell_line;
    int scroll_offset;
    int history_count;
    char** line_history;
    // int  last_status; // Last command exit status
    // int   tty_fd; // Terminal file descriptor
    // pid_t shell_pgid; // Shell process group ID
    // pid_t last_pgid;    // Last foreground process group ID
    // pid_t pipeline_pgid; // Current pipeline process group ID
    // char cwd[512];            // Current working directory
    // History history; // Command history
    // VarTable *vars; // Hash table for variables
} ShellContext;


void clear_line_no_prompt(ShellContext *shell);

void clear_line(ShellContext *shell);

void draw_prompt(void);

int process_scancode(ShellContext *shell, uint8_t scancode);

int process_cmd(ShellContext *shell, char *cmd);

void init_shell_lines(ShellContext *shell);

void shift_right(char* buf, size_t pos, size_t len);

void shift_left(char* buf, size_t pos, size_t len);

void clamp_n_scroll(ShellContext *shell);

void fb_draw_stringsh(const char* str, int len, uint32_t fg, uint32_t bg, struct ShellContext *shell);

void scroll_on_newline(ShellContext *shell);

void scroll_on_wrap(ShellContext *shell);


// void scroll_history_up(ShellContext *shell);

// void scroll_history_down(ShellContext *shell);

// void redraw_from_history(ShellContext *shell);

#endif