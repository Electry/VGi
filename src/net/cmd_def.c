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

#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

static void cmd_print_help(char *arg) {
    vgi_cmd_send_msg("Available commands:\n"
                     "  help                                                   - Show this text\n"
                     "  info       (app|g|rt|cs|dss|tex|scenes|misc|cg|dev)    - Show VGi section\n"
                     "\n"
                     "Memory RW:\n"
                     "  r          <vaddr>    [range]                          - Read dword (+range) at vaddr\n"
                     "  w          <vaddr>    <vgexpr>                         - Write dword to vaddr\n"
                     "  s          (e|h|b|a)  <vgexpr>                         - Search data in eboot/heap/memblocks/all\n"
                     "  i          <vaddr>    <vgexpr>                         - Inject data to vaddr\n"
                     "  release    <id>                                        - Release injected data\n"
                     "\n"
                     "Hooks (experimental):\n"
                     "  h          <vaddr>    [f]                              - Hook function and dump it's arguments when called\n"
                     "                                                           (f = dump stack as well)\n"
                     "  hd         <vaddr>    <register>    <offset>           - Hook function, dump it's arguments, stack, vfp and\n"
                     "                                                           dereference *(DWORD *)(R<register> + <offset>)\n"
                     "  hr                                                     - Release function hook\n"
                     "\n"
                     "Gxm (experimental):\n"
                     "  listparams <gxp>                                       - List GXP shader parameters\n"
                     "  dumpparams <gxp>                                       - Dump GXP shader uniform buffer\n");
}

static void cmd_info(char *arg) {
    if (!strncmp(arg, "app", 3)) {
        vgi_dump_appinfo();
    } else if (!strncmp(arg, "g", 1)) {
        vgi_dump_graphics();
    } else if (!strncmp(arg, "rt", 2)) {
        vgi_dump_graphics_rt();
    } else if (!strncmp(arg, "cs", 2)) {
        vgi_dump_graphics_cs();
    } else if (!strncmp(arg, "dss", 3)) {
        vgi_dump_graphics_dss();
    } else if (!strncmp(arg, "tex", 3)) {
        vgi_dump_graphics_tex();
    } else if (!strncmp(arg, "scenes", 6)) {
        vgi_dump_graphics_scenes();
    } else if (!strncmp(arg, "misc", 4)) {
        vgi_dump_graphics_misc();
    } else if (!strncmp(arg, "cg", 2)) {
        vgi_dump_graphics_cg();
    } else if (!strncmp(arg, "dev", 3)) {
        vgi_dump_device();
    } else {
        vgi_cmd_send_msg("Unknown section!\n");
    }
}

static const vgi_cmd_definition_t CMD_DEFINITIONS[] = {
    {.name = "help", .executor = &cmd_print_help},
    {.name = "info", .executor = &cmd_info},

    {.name = "r", .executor = &vgi_cmd_mem_read_dword},
    {.name = "w", .executor = &vgi_cmd_mem_write_dword},
    {.name = "s",  .executor = &vgi_cmd_mem_search},
    {.name = "i",  .executor = &vgi_cmd_mem_inject_data},
    {.name = "release",  .executor = &vgi_cmd_mem_release_data},

    {.name = "hr", .executor = &vgi_cmd_hook_dump_args_release},
    {.name = "hd", .executor = &vgi_cmd_hook_dump_deref_hook},
    {.name = "h", .executor = &vgi_cmd_hook_dump_args_hook},

    {.name = "listparams",  .executor = &vgi_cmd_gxm_listparams},
    {.name = "dumpparams",  .executor = &vgi_cmd_gxm_dumpparams}
};

const vgi_cmd_definition_t *vgi_cmd_get_definition(char *cmd_name) {
    for (unsigned int i = 0; i < COUNT_OF(CMD_DEFINITIONS); i++) {
        if (!strcmp(cmd_name, CMD_DEFINITIONS[i].name)) {
            return &(CMD_DEFINITIONS[i]);
        }
    }

    return NULL;
}

uint32_t vgi_cmd_parse_address(char *arg, char **end) {
    char *endptr = NULL;
    uint32_t pos = 0;

    // Peek ':'
    while (arg[pos] != '\0' && arg[pos] != ':') { pos++; }

    // Relative address
    if (arg[pos] == ':') {
        pos = 0;

        // Parse segment
        uint32_t segment = strtoul(&arg[pos], &endptr, 10); // always base 10
        if (endptr == &arg[pos]) {
            return 0;
        }
        pos += endptr - &arg[pos];

        // Enforce ':'
        if (arg[pos] != ':')
            return 0;
        pos++;

        // Parse offset
        uint32_t offset = strtoul(&arg[pos], &endptr, 16);
        if (endptr == &arg[pos]) {
            return 0;
        }

        if (segment != 0 && segment != 1)
            return 0;

        // Calculate absolute addr
        uint32_t base = (uint32_t)g_app_sce_info.segments[segment].vaddr;
        uint32_t addr = base + offset;
        if (addr > (uint32_t)g_app_sce_info.segments[segment].vaddr + (uint32_t)g_app_sce_info.segments[segment].memsz) {
            return 0;
        }

        *end = endptr;
        return addr;
    }
    else {
        uint32_t addr = strtoul(arg, &endptr, 16);
        if (endptr == arg) {
            return 0;
        }

        if (addr > 0xA0000000 || addr < 0x10000000) {
            return 0;
        }

        *end = endptr;
        return addr;
    }

    return 0;
}

uint32_t vgi_cmd_addr_to_offset(uint32_t addr) {
    if (addr > (uint32_t)g_app_sce_info.segments[0].vaddr
            && addr < (uint32_t)g_app_sce_info.segments[0].vaddr + (uint32_t)g_app_sce_info.segments[0].memsz) {
        return addr - (uint32_t)g_app_sce_info.segments[0].vaddr;
    }

    return 0;
}
