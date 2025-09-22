#include "command.h"
#include "mem.h"
#include "serial.h"
//-----------------------------------------------------------------------------
//                       FREE  ROUTINES
//-----------------------------------------------------------------------------
void free_command_list(Command **cmds, int num_cmds) {
    sfprint("freeing command list\n");
    if (!cmds) return;
    for (int i = 0; i < num_cmds; ++i) {
        Command *cmd = cmds[i];
        if (cmd && cmd->argv && cmd->argv[0]) {
            //LOG(LOG_LEVEL_INFO, "Freeing command[%d]: '%s'", i, cmd->argv[0]);
        } else {
            //LOG(LOG_LEVEL_INFO, "Freeing command[%d]: <invalid>", i);
        }
        if (cmd) {
            free_command(cmd);
        }
    }
    tfree(cmds);
    sfprint("Command list freed\n");
}

void free_command(Command *cmd) {
    if (!cmd) return;
    if (cmd->argv) {
        for (int i = 0; i < cmd->argc; ++i) {
            tfree(cmd->argv[i]);
        }
        tfree(cmd->argv);
    }
    tfree(cmd->input_file);
    //free(cmd->output_file);
    // free(cmd->append_file);
    // free(cmd->error_file);
    // free(cmd->heredoc);
    // free(cmd->cwd_override);
    // free(cmd->raw_input);
    tfree(cmd);
}
