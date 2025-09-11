#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>
#include "kbd.h"


#define INPUT_SIZE 1024

typedef struct {
    char input[INPUT_SIZE];   // User input buffer
    _Bool running;              // Shell loop control flag
    // int  last_status; // Last command exit status
    // int   tty_fd; // Terminal file descriptor
    // pid_t shell_pgid; // Shell process group ID
    // pid_t last_pgid;    // Last foreground process group ID
    // pid_t pipeline_pgid; // Current pipeline process group ID
    // char cwd[512];            // Current working directory
    // History history; // Command history
    // VarTable *vars; // Hash table for variables
} ShellContext;



#define LINEBUFF_SIZE 128

int process_scancode(ShellContext *shell, uint8_t scancode);

int process_cmd(ShellContext *shell, char *cmd);


void init_shell_lines(void);

#endif