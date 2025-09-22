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
        // // 3) Pipe: end current command, start next
        // if (c == '|' && !in_single && !in_double) {
        //     // flush any pending word
        //     if (buff_index > 0) {
        //         token_buff[buff_index] = '\0';
        //         if (arg_index < MAX_ARGS - 1) {
        //             current->argv[arg_index++] = strdup(token_buff);
        //         }
        //         buff_index = 0;
        //     }

        //     // terminate current->argv
        //     current->argv[arg_index] = NULL;
        //     current->argc = arg_index;

        //     // store command
        //     if (cmd_index < MAX_CMDS) {
        //         cmds[cmd_index++] = current;
        //     } else {
        //         //LOG(LOG_LEVEL_ERR, "Too many commands, discarding extra");
        //         free_command(current);
        //         aborted = true;
        //         break;
        //     }

        //     // allocate next Command
        //     current = cralloc(1, sizeof(Command));
        //     if (!current) { aborted = true; break; }
        //     current->argv = cralloc(MAX_ARGS, sizeof(char *));
        //     if (!current->argv) { tfree(current); aborted = true; break; }

        //     arg_index = 0;
        //     p++;
        //     continue;
        // }

        // // 4) Redirection: >, >>, <
        // if ((c == '>' || c == '<') && !in_single && !in_double) {
        //     char chevron = c;
        //     bool is_append = (c == '>' && p[1] == '>');
        //     int specified_fd = -1;

        //     // If the token buffer is all digits, consume it as a FD spec
        //     if (buff_index > 0) {
        //         bool all_digits = true;
        //         for (int i = 0; i < buff_index; i++) {
        //             if (token_buff[i] < '0' || token_buff[i] > '9') {
        //                 all_digits = false;
        //                 break;
        //             }
        //         }
        //         if (all_digits) {
        //             specified_fd = atoi(token_buff);
        //             buff_index = 0;
        //         } else {
        //             // flush it as a real argv word
        //             token_buff[buff_index] = '\0';
        //             if (arg_index < MAX_ARGS - 1) {
        //                 current->argv[arg_index++] = strdup(token_buff);
        //             }
        //             buff_index = 0;
        //         }
        //     }

        //     // skip the chevron(s)
        //     p += is_append ? 2 : 1;

        //     // skip spaces before filename
        //     while (*p && isspace((unsigned char)*p)) p++;

        //     // parse the filename (respecting quotes)
        //     buff_index = 0;
        //     while (*p && (!isspace((unsigned char)*p) || in_single || in_double)) {
        //         if (*p == '\'' && !in_double) {
        //             in_single = !in_single;
        //             p++;
        //         } else if (*p == '"' && !in_single) {
        //             in_double = !in_double;
        //             p++;
        //         } else {
        //             if (buff_index < (int)sizeof(token_buff) - 1) {
        //                 token_buff[buff_index++] = *p;
        //             }
        //             p++;
        //         }
        //     }
        //     token_buff[buff_index] = '\0';
        //     char *filename = strdup(token_buff);
        //     buff_index = 0;

        //     // assign to input/output as appropriate
        //     if (chevron == '<') {
        //         current->input_file = filename;
        //         current->input_fd = (specified_fd != -1) ? specified_fd : 0;
        //     } else if (chevron == '>' && is_append) {
        //         current->append_file = filename;
        //         current->output_fd = (specified_fd != -1) ? specified_fd : 1;
        //     } else {
        //         current->output_file = filename;
        //         current->output_fd = (specified_fd != -1) ? specified_fd : 1;
        //     }

        //     // do not push filename to argv
        //     continue;
        // }

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
    if (!segments) return;
    Command **cmds;
    int num_cmds = 0;    
    for (int i = 0; segments[i]; ++i) {
        num_cmds = 0;
        cmds = parse_commands(segments[i], &num_cmds);
        sfprint("parse_commands returned %d command(s)\n", num_cmds);

        // Validate command list before doing anything
        bool valid = true;
        if (!cmds || num_cmds <= 0) {
            valid = false;
        } else {
            for (int j = 0; j < num_cmds; ++j) {
                if (!cmds[j]) {
                    sfprint("cmds[%d] is NULL", j);
                    valid = false;
                    break;
                }
                if (!cmds[j]->argv || !cmds[j]->argv[0]) {
                   sfprint("cmds[%d] has invalid argv", j);
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
            asm volatile("cli; hlt");
            break;
        }
        else if (str_eq(cmd_name, "LS") || str_eq(cmd_name, "ls")) {
            clear_line_no_prompt(shell);
            shell->shell_line++;
            fs_list_files(shell);
            //free_command_list(cmds, num_cmds);
            break;
        }
        else if (str_eq(cmd_name, "cat") || str_eq(cmd_name, "CAT")) {
            
            if (cmds[0]->argv[1]) {
                clear_line_no_prompt(shell);
                //shell->shell_line++;
                print_file(cmds[0]->argv[1], shell);
                //free_command_list(cmds, num_cmds);
                break;
            }
        } else { 
            draw_prompt();
            fbprintf("command: '%s' es no bueno", cmd_name);
            fb_draw_string("\n", FG, BG);
            draw_prompt();
            //free_command_list(cmds, num_cmds);
        }
    }
    sfprint("freeing segments\n");
    free_segments(segments);
    free_command_list(cmds, num_cmds);
    clear_line(shell);
}

// Note: whitespace trimming deferred to parse_commands()
// This function only handles quote-aware semicolon splitting
char **split_on_semicolons(const char *input) {
    sfprint("splitting input\n");
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
    sfprint("seg count: %8", seg_count);
    segments[seg_count] = NULL; // NULL-terminate the array

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

