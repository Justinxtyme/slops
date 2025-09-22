#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include "kbd.h"
#include "types.h"

#define INPUT_SIZE 1024
#define LINEBUFF_SIZE 128
#define MAX_HISTORY_LINES 1024

typedef struct ShellContext {
    char input[INPUT_SIZE];   // User input buffer
    _Bool running;              // Shell loop control flag
    int shell_line;
    int scroll_offset;
    int history_count;
    char *line_history[MAX_HISTORY_LINES];
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

#endif