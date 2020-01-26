#include <vitasdk.h>
#include <taihen.h>

#include "../main.h"
#include "cmd.h"
#include "cmd_def.h"
#include "../interpreter/interpreter.h"

#define MAX_HOOK_MSG_COUNT 10

static uintptr_t g_hook_vaddr;
static tai_hook_ref_t g_hook_hookref;
static SceUID g_hook_hookid = -1;
static bool g_hook_dump_stack = false;
static uint32_t g_hook_antiflood = 0;

static uint32_t _hook_dump_args(uint32_t R0, uint32_t R1, uint32_t R2, uint32_t R3, uint32_t stack) {
    uint32_t SP = (uint32_t)&stack;
    uint32_t LR, S[4];
    asm volatile ("MOV %0, LR\n" : "=r" (LR));

    for (int i = 0; i < 4; i++) {
        asm volatile ("VMOV %0, S0\t\n" : "=r" (S[i]));
    }

    char res_msg[256];
    char stack_msg[256];
    uint32_t ret = TAI_CONTINUE(uint32_t, g_hook_hookref, R0, R1, R2, R3);
    if (g_hook_antiflood >= MAX_HOOK_MSG_COUNT) {
        if (g_hook_antiflood == MAX_HOOK_MSG_COUNT)
            vgi_cmd_send_msg("Remaining messages hidden...\n");
        g_hook_antiflood++;
        return ret;
    }

    snprintf(res_msg, sizeof(res_msg), "0x%08X:  RET: 0x%lX\n"
                                       "    Rx:  0x%08lX, 0x%08lX, 0x%08lX, 0x%08lX\n"
                                       "    VFP: 0x%08lX, 0x%08lX, 0x%08lX, 0x%08lX\n"
                                       "    SP:  0x%08lX   LR:  0x%08lX\n",
                                       g_hook_vaddr, ret,
                                       R0, R1, R2, R3,
                                       S[0], S[1], S[2], S[3],
                                       SP, LR);
    vgi_cmd_send_msg(res_msg);

    if (g_hook_dump_stack) {
        snprintf(stack_msg, sizeof(stack_msg), "Stack:\n"
                                               "     0x0: 0x%08lX 0x%08lX 0x%08lX 0x%08lX\n"
                                               "    0x10: 0x%08lX 0x%08lX 0x%08lX 0x%08lX\n"
                                               "    0x20: 0x%08lX 0x%08lX 0x%08lX 0x%08lX\n\n",
                                       *(uint32_t *)(SP), *(uint32_t *)(SP + 4), *(uint32_t *)(SP + 8), *(uint32_t *)(SP + 12),
                                       *(uint32_t *)(SP + 16), *(uint32_t *)(SP + 20), *(uint32_t *)(SP + 24), *(uint32_t *)(SP + 28),
                                       *(uint32_t *)(SP + 32), *(uint32_t *)(SP + 36), + *(uint32_t *)(SP + 40), *(uint32_t *)(SP + 44));
        vgi_cmd_send_msg(stack_msg);
    }

    g_hook_antiflood++;
    return ret;
}

void vgi_cmd_hook_dump_args_hook(char *arg) {
    char *endptr = NULL;
    char res_msg[RES_MSG_LEN];

    if (g_hook_hookid >= 0) {
        vgi_cmd_send_msg("Function already hooked!\n");
        return;
    }

    uint32_t addr = vgi_cmd_parse_address(arg, &endptr);
    if (!addr) {
        vgi_cmd_send_msg("Invalid format for arg 1.\n");
        return;
    }

    uint32_t pos = 0;
    while (isspace(endptr[pos])) pos++;
    g_hook_dump_stack = endptr[pos] == 'f';

    // Calc offset
    uint32_t offset = vgi_cmd_addr_to_offset(addr);
    if (!offset) {
        snprintf(res_msg, RES_MSG_LEN, "Invalid addr 0x%lx.\n", addr);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    g_hook_antiflood = 0;
    g_hook_vaddr = addr;
    g_hook_hookid = taiHookFunctionOffset(&g_hook_hookref, g_app_tai_info.modid, 0, offset, 1, _hook_dump_args);
    snprintf(res_msg, RES_MSG_LEN, "Hook offset 0x%lX, UID: 0x%X\n", offset, g_hook_hookid);
    vgi_cmd_send_msg(res_msg);
}

void vgi_cmd_hook_dump_args_release(char *arg) {
    char res_msg[RES_MSG_LEN];

    if (g_hook_hookid < 0) {
        vgi_cmd_send_msg("Function not hooked!\n");
        return;
    }

    taiHookRelease(g_hook_hookid, g_hook_hookref);
    g_hook_hookid = -1;

    snprintf(res_msg, RES_MSG_LEN, "Released. Last hook (0x%X) called %ld times.\n", g_hook_vaddr, g_hook_antiflood);
    vgi_cmd_send_msg(res_msg);
}
