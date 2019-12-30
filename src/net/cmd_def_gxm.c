#include <vitasdk.h>
#include <taihen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "../main.h"
#include "cmd.h"
#include "cmd_def.h"
#include "../interpreter/interpreter.h"

void vgi_cmd_gxm_listparams(char *arg) {
    char *endptr = arg;
    char res_msg[RES_MSG_LEN];

    // Parse program ptr
    uint32_t addr = vgi_cmd_parse_address(arg, &endptr);
    if (!addr) {
        vgi_cmd_send_msg("Invalid format for arg 1.\n");
        return;
    }

    const SceGxmProgram *program = (SceGxmProgram *)addr;
    int program_size = sceGxmProgramGetSize(program);
    if (program_size <= 0) {
        vgi_cmd_send_msg("Invalid program!\n");
        return;
    }

    int param_count = sceGxmProgramGetParameterCount(program);
    if (param_count <= 0) {
        snprintf(res_msg, RES_MSG_LEN, "Program 0x%lX has no parameters.\n", addr);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    for (int i = 0; i < param_count; i++) {
        const SceGxmProgramParameter *param = sceGxmProgramGetParameter(program, i);
        const char *name = sceGxmProgramParameterGetName(param);

        snprintf(res_msg, RES_MSG_LEN, "0x%lX: '%s' #0x%X\n",
                (uint32_t)param, name == NULL ? "<unk>" : name, sceGxmProgramParameterGetResourceIndex(param));
        vgi_cmd_send_msg(res_msg);
    }
}

void vgi_cmd_gxm_dumpparams(char *arg) {
    char *endptr = arg;
    char res_msg[RES_MSG_LEN];

    // Parse program ptr
    uint32_t addr = vgi_cmd_parse_address(arg, &endptr);
    if (!addr) {
        vgi_cmd_send_msg("Invalid format for arg 1.\n");
        return;
    }

    const SceGxmProgram *program = (SceGxmProgram *)addr;
    int program_size = sceGxmProgramGetSize(program);
    if (program_size <= 0) {
        vgi_cmd_send_msg("Invalid program!\n");
        return;
    }

    int param_count = sceGxmProgramGetParameterCount(program);
    if (param_count <= 0) {
        snprintf(res_msg, RES_MSG_LEN, "Program 0x%lX has no parameters.\n", addr);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    vgi_cmd_send_msg("Waiting for program...\n");
    vgi_dump_graphics_unibuf(program);
}
