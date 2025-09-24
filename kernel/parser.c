#include <stdbool.h>
#include "command.h"
#include "parser.h"
#include "types.h"
#include "string.h"
#include "mem.h"
#include "framebuffer.h"
#include "fat.h"

#define MAX_ARGS 64
#define MAX_CMDS 16

Command **parse_commands(const char *input, int *num_cmds) {
    sfprint("parsing commands\n");
    Command **cmds = cralloc(MAX_CMDS, sizeof(Command *));
    if (!cmds) {
        if (num_cmds) *num_cmds = 0;
        return NULL;
    }

    int cmd_index = 0;
    int arg_index = 0;
    int buff_index = 0;

    char token_buff[1024];
    bool in_single = false, in_double = false;
    const char *p = input;
    bool aborted = false;

    // Allocate first Command
    Command *current = cralloc(1, sizeof(Command));
    if (!current) {
        tfree(cmds);
        if (num_cmds) *num_cmds = 0;
        return NULL;
    }
    current->argv = cralloc(MAX_ARGS, sizeof(char *));
    if (!current->argv) {
        tfree(current);
        tfree(cmds);
        if (num_cmds) *num_cmds = 0;
        return NULL;
    }

    while (*p) {
        char c = *p;
        sfprint("char: %c\n", c);
        // 1) Escape sequence: copy next char verbatim
        if (c == '\\' && p[1]) {
            if (buff_index < (int)sizeof(token_buff) - 1) {
                token_buff[buff_index++] = p[1];
            }
            p += 2;
            continue;
        }
        // 2) Quote toggles
        if (c == '\'' && !in_double) {
            in_single = !in_single;
            p++;
            continue;
        }
        if (c == '"' && !in_single) {
            in_double = !in_double;
            p++;
            continue;
        }

        // 5) Whitespace: flush token
        if ((c == ' ') && !in_single && !in_double) {
            if (buff_index > 0) {
                token_buff[buff_index] = '\0';
                if (arg_index < MAX_ARGS - 1) {
                    current->argv[arg_index++] = strdupe(token_buff);
                    sfprint("\ntoken buffered: %s\n", token_buff);
                }
                buff_index = 0;
            }
            p++;
            continue;
        }
        // 6) Regular character: accumulate
        if (buff_index < (int)sizeof(token_buff) - 1) {
            token_buff[buff_index++] = c;
        }
        p++;
    }
    // Final flush if not aborted
    if (!aborted) {
        sfprint("Not aborted\n");
        if (buff_index > 0) {
            token_buff[buff_index] = '\0';
            if (arg_index < MAX_ARGS - 1) {
                current->argv[arg_index++] = strdupe(token_buff);
                sfprint("\ntoken buffered: %s\n", token_buff);
            }
        }
        current->argv[arg_index] = NULL;
        current->argc = arg_index;

        if (cmd_index < MAX_CMDS) {
            cmds[cmd_index++] = current;
        } else {
            //LOG(LOG_LEVEL_ERR, "Final command exceeds MAX_CMDS, discarding");
            free_command(current);
        }
    }
    if (num_cmds) *num_cmds = cmd_index;
    sfprint("returning commands\n");
    return cmds;
}   


/*=================================process_input_segments=====================================
processes expanded input, splitting at semicolons for */
void process_input_segments(ShellContext *shell, const char *expanded_input) {
    sfprint("processing input\n");
    char **segments = split_on_semicolons(expanded_input);
    if (!segments) {
        draw_prompt();
        // ends up skipping a line on empty input, instead of 
        // moving to the next line. --shell_line is a bandaid
        // until i can figure out whats happening
        --shell->shell_line;
        tfree(segments);
        return;
    } 
    Command **cmds;
    int num_cmds = 0;    
    for (int i = 0; segments[i]; ++i) {
        sfprint("segment %d\n", i);
        num_cmds = 0;
        cmds = parse_commands(segments[i], &num_cmds);
        sfprint("parse_commands returned %d command(s)\n", num_cmds + 1);
        for (int a = 0; a < num_cmds; ++a) {
            sfprint("command: %s\n", segments[a]);
        }
        // Validate command list before doing anything
        bool valid = true;
        if (!cmds || num_cmds <= 0) {
            valid = false;
        } else {
            for (int j = 0; j < num_cmds; ++j) {
                if (!cmds[j]) {
                    sfprint("cmds[%d] is NULL\n", j);
                    valid = false;
                    break;
                }
                if (!cmds[j]->argv || !cmds[j]->argv[0]) {
                   sfprint("cmds[%d] has invalid argv\n", j);
                    valid = false;
                    break;
                }
            }
        }
        if (!valid) {
           // LOG(LOG_LEVEL_WARN, "Skipping invalid command segment: '%s'", segments[i]);
            free_command_list(cmds, num_cmds);
            continue;
        }
        const char *cmd_name = cmds[0]->argv[0];

        sfprint("command: %s\n", cmd_name);
        //Built-in: exit
        if (cst_strcmp(cmd_name, "exit") || cst_strcmp(cmd_name, "quit") || cst_strcmp(cmd_name, "q")) {
            draw_prompt();
            fb_draw_string("Quitting", FG, BG);
            shell->running = 0;
            free_command_list(cmds, num_cmds);
            free_segments(segments);
            
            for (int i = 0; i < MAX_HISTORY_LINES; ++i) {
                if (shell->line_history[i]) {
                    tfree(shell->line_history[i]);
                    shell->line_history[i] = NULL;
                }
            }
            tfree(shell->line_history);
            shell->line_history = NULL;
            tfree(shell);
            asm volatile("cli; hlt");
            break;
        }
        else if (str_eq(cmd_name, "ls") || str_eq(cmd_name, "LS")) {
            clear_line_no_prompt(shell);
            //shell->shell_line++;
            fs_list_files(shell);
            sfprint("Returned from free_command_list\n");
            break;
        }
        else if (str_eq(cmd_name, "cat") || str_eq(cmd_name, "CAT")) {            
            if (cmds[0]->argv[1]) {
                clear_line_no_prompt(shell);
                //shell->shell_line++;
                if (!print_file(cmds[0]->argv[1], shell)) {
                    //sfprint("\n\n\nshell_line: %d\n", shell->shell_line);
                    draw_prompt();
                    fb_draw_stringsh("No file, or no data to read!", 28, FG, BG, shell);
                    clamp_n_scroll(shell);
                    shell->shell_line++;
                    clamp_n_scroll(shell);
                    break;
                }
                clamp_n_scroll(shell);
                shell->shell_line++;
                clamp_n_scroll(shell);
                break;
            } else {
                draw_prompt();
                fb_draw_stringsh("No argument detected", 20, FG, BG, shell);
                clamp_n_scroll(shell);
                break;
            }
        } 
        else if (str_eq(cmd_name, "")) {
            draw_prompt();
            break;

        } else { 
            draw_prompt();
            fbprintf(shell, "command: '%s' es no bueno", cmd_name);
            //fb_draw_string("\n", FG, BG);
            clamp_n_scroll(shell);
            
        }
    }
    //sfprint("freeing segments\n");
    free_segments(segments);
    //sfprint("Cleared segment, freeing command list\n");
    free_command_list(cmds, num_cmds);
    clear_line(shell);
    //sfprint("Cleared line\n");
}

// Note: whitespace trimming deferred to parse_commands()
// This function only handles quote-aware semicolon splitting
char **split_on_semicolons(const char *input) {
    //sfprint("splitting input\n");
    if (!input) return NULL; // Defensive: null input yields null output

    size_t len = custom_strlen(input);
    if (len == 0) return NULL; // Empty input yields null output

    // Make a modifiable copy of the input string — we will insert NULs here
    sfprint("duping %s\n", input);
    char *copy = strdupe(input);
    sfprint("Duped input: %s\n", copy);
    if (!copy) return NULL;

    // Allocate space for output segments (+1 for NULL terminator)
    // Worst case: every character is a delimiter, so len+1 segments
    char **segments = cralloc(len + 2, sizeof(char *));
    sfprint("Allocing for segments\n");
    if (!segments) {
        tfree(copy);
        return NULL;
    }
    sfprint("Alloc'd segments\n");
    int seg_count = 0;
    char *start = copy; // Marks the beginning of the current segment
    char *p = copy;     // Current scan position
    char quote = '\0';  // Quote state: '\0' means unquoted, otherwise holds quote char
    int escape = 0;     // Escape state: 1 means next char is literal

    while (*p) {
        sfprint("while\n");
        if (escape) {
            /*
             * Previous char was a backslash, so this char is taken literally.
             * This bypasses quote toggling and delimiter checks.
             */
            escape = 0; // Reset escape after consuming one char
            ++p;
            continue;
        }
        if (*p == '\\') {
            /*
             * Found a backslash — set escape so the next char is treated literally.
             * Do not include '\' itself in any special logic.
             */
            escape = 1;
            // Optionally: memmove(p, p+1, strlen(p)); to strip '\' from output
            ++p;
            continue;
        }
        if (*p == '\'' || *p == '"') {
            /*
             * Quote handling:
             * - If unquoted, enter quoted mode using this char as the delimiter.
             * - If already quoted with the same char, close quote.
             * Inside quotes, delimiters like ';' are ignored.
             */
            if (quote == '\0') {
                quote = *p; // Enter quoted mode
            } else if (quote == *p) {
                quote = '\0'; // Exit quoted mode
            }
            ++p;
            continue;
        }
        if (((*p == ';') || (*p == '\n')) && (quote == '\0')) {
            /*
             * We found a delimiter (semicolon or newline) while NOT in quotes
             * and NOT escaped (escape is always 0 here because that branch returns early).
             */
            if (p > start && *(p - 1) == '\r') {
                *(p - 1) = '\0'; // Trim carriage return from CRLF endings
            }
            *p = '\0'; // Terminate the current segment string

            if (*start) {
                // Only store non-empty segments
                sfprint("\n\n\nDuping segment %d\n", seg_count);
                segments[seg_count++] = strdupe(start);       
            }
            start = p + 1; // New segment starts after the delimiter
            ++p;
            continue;
        }
        // Default: just advance to next character
        ++p;
    }
    // After loop ends, handle the final segment (if non-empty)
    if (*start) {
        segments[seg_count++] = strdupe(start);
    }
    sfprint("seg count: %8\n", seg_count);
    segments[seg_count] = NULL; // NULL-terminate the array
    for (int i = 0; i < seg_count; ++i) {
        sfprint("segments[%d] : %s\n", i, segments[i]);
    }
    tfree(copy);
    return segments;
}

void free_segments(char **segments) {
    if (!segments) return;
    for (int i = 0; segments[i]; ++i) {
        tfree(segments[i]);
    }
    tfree(segments);
}

