#ifndef PARSER_H
#define PARSER_H

#include "command.h"
#include "shell.h"

Command **parse_commands(const char *input, int *num_cmds);

char **split_on_semicolons(const char *input);

void process_input_segments(ShellContext *shell, const char *expanded_input);

void free_segments(char **segments);

#endif