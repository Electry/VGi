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

#define MAX_INJECT_NUM 128
static uint8_t g_inject_num = 0;
static SceUID g_inject[MAX_INJECT_NUM];

static float as_float(uint32_t dword) {
    float float_repr;
    memcpy(&float_repr, &dword, sizeof(float));
    return float_repr;
}

void vgi_cmd_mem_read_dword(char *arg) {
    char *endptr = NULL, *endptr2 = NULL;
    char res_msg[RES_MSG_LEN];

    uint32_t addr = vgi_cmd_parse_address(arg, &endptr);
    if (!addr) {
        vgi_cmd_send_msg("Invalid format for arg 1.\n");
        return;
    }

    uint32_t count = strtoul(endptr, &endptr2, 0);
    if (endptr2 == endptr)
        count = 1;

    uint32_t dword;
    for (int i = 0; i < count; i++) {
        dword = *(uint32_t *)addr;

        snprintf(res_msg, RES_MSG_LEN, "0x%08lX: 0x%08lX, %ld, %f\n", addr, dword, dword, as_float(dword));
        vgi_cmd_send_msg(res_msg);

        addr += sizeof(uint32_t);
    }
}

void vgi_cmd_mem_write_dword(char *arg) {
    char *endptr = NULL;
    char res_msg[RES_MSG_LEN];

    uint32_t addr = vgi_cmd_parse_address(arg, &endptr);
    if (!addr) {
        vgi_cmd_send_msg("Invalid format for arg 1.\n");
        return;
    }

    // Evaluate expression
    uint32_t pos = 0;
    intp_value_t patch_data;
    intp_status_t intp_ret = intp_evaluate(endptr, (uint32_t *)&pos, &patch_data);
    if (intp_ret.code != INTP_STATUS_OK
            || (patch_data.size != 4 && patch_data.size != 2 && patch_data.size != 1)) {
        snprintf(res_msg, RES_MSG_LEN, "Invalid format for arg 2: %d\n", intp_ret.code);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    if (patch_data.size == 4)
        *(uint32_t *)addr = patch_data.data.uint32;
    else if (patch_data.size == 2)
        *(uint16_t *)addr = (uint16_t)patch_data.data.uint32;
    else if (patch_data.size == 1)
        *(uint8_t *)addr = (uint8_t)patch_data.data.uint32;

    snprintf(res_msg, RES_MSG_LEN, "Write 0x%08lX OK\n", addr);
    vgi_cmd_send_msg(res_msg);
}

void vgi_cmd_mem_inject_data(char *arg) {
    char *endptr = NULL;
    char res_msg[RES_MSG_LEN];

    if (g_inject_num >= MAX_INJECT_NUM) {
        vgi_cmd_send_msg("Too many injects! Release one!\n");
        return;
    }

    uint32_t addr = vgi_cmd_parse_address(arg, &endptr);
    if (!addr) {
        vgi_cmd_send_msg("Invalid format for arg 1.\n");
        return;
    }

    // Evaluate expression
    uint32_t pos = 0;
    intp_value_t patch_data;
    intp_status_t intp_ret = intp_evaluate(endptr, (uint32_t *)&pos, &patch_data);
    if (intp_ret.code != INTP_STATUS_OK
            || (patch_data.size != 4 && patch_data.size != 2 && patch_data.size != 1)) {
        snprintf(res_msg, RES_MSG_LEN, "Invalid format for arg 2: %d\n", intp_ret.code);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    // Find free space
    int fre = -1;
    for (int i = 0; i < MAX_INJECT_NUM; i++) {
        if (g_inject[i] <= 0) {
            fre = i;
            break;
        }
    }
    if (fre == -1) {
        vgi_cmd_send_msg("Too many injects! Release one!\n");
        return;
    }

    // Calc offset
    uint32_t offset = vgi_cmd_addr_to_offset(addr);
    if (!offset) {
        snprintf(res_msg, RES_MSG_LEN, "Invalid addr 0x%lx.\n", addr);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    g_inject[fre] = taiInjectData(g_app_tai_info.modid, 0, offset, patch_data.data.raw, patch_data.size);
    if (g_inject[fre] <= 0) {
        snprintf(res_msg, RES_MSG_LEN, "Inject %d:0x%lX FAILED: 0x%X.\n", 0, offset, g_inject[fre]);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    snprintf(res_msg, RES_MSG_LEN, "Inject %d:0x%lX #%d OK.\n", 0, offset, fre);
    vgi_cmd_send_msg(res_msg);
    g_inject_num++;
}

void vgi_cmd_mem_release_data(char *arg) {
    char *endptr = NULL;
    char res_msg[RES_MSG_LEN];

    uint32_t id = strtoul(arg, &endptr, 0);
    if (arg == endptr) {
        vgi_cmd_send_msg("Invalid format for arg 1.\n");
        return;
    }

    if (id > MAX_INJECT_NUM || g_inject[id] <= 0) {
        snprintf(res_msg, RES_MSG_LEN, "Invalid id %ld.\n", id);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    taiInjectRelease(g_inject[id]);
    snprintf(res_msg, RES_MSG_LEN, "Released id %ld.\n", id);
    vgi_cmd_send_msg(res_msg);
    g_inject[id] = -1;
    g_inject_num--;
}

static void scan_memory(void *base, uint32_t size, intp_value_t search_data) {
    char res_msg[RES_MSG_LEN];

    for (uint32_t vaddr = (uint32_t)base; vaddr < (uint32_t)base + size - search_data.size - 1; vaddr++) {
        bool found = true;
        for (int y = 0; y < search_data.size; y++) {
            uint8_t byte = *(uint8_t *)(vaddr + y);
            if (byte != search_data.data.raw[y]) {
                found = false;
                break;
            }
        }

        if (found) {
            snprintf(res_msg, RES_MSG_LEN, "  0x%08lX: 0x%08lX ..., %ld, %f\n",
                    vaddr, search_data.data.uint32, search_data.data.uint32, as_float(search_data.data.uint32));
            vgi_cmd_send_msg(res_msg);
        }
    }
}

void vgi_cmd_mem_search(char *arg) {
    char *endptr = arg;
    uint32_t pos = 0;
    char res_msg[RES_MSG_LEN];

    bool search_eboot = false, search_heap = false, search_blocks = false;
    while (isspace(endptr[pos])) pos++;
    if (endptr[pos] == 'e') {
        search_eboot = true;
    } else if (endptr[pos] == 'h') {
        search_heap = true;
    } else if (endptr[pos] == 'b') {
        search_blocks = true;
    } else if (endptr[pos] == 'a') {
        search_eboot = search_heap = search_blocks = true;
    } else {
        vgi_cmd_send_msg("Invalid format for arg 1.\n");
        return;
    }
    pos++;

    // Evaluate expression
    intp_value_t patch_data;
    intp_status_t intp_ret = intp_evaluate(endptr, (uint32_t *)&pos, &patch_data);
    if (intp_ret.code != INTP_STATUS_OK) {
        snprintf(res_msg, RES_MSG_LEN, "Invalid format for arg 2: %d\n", intp_ret.code);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    // Scan .elf segments
    if (search_eboot) {
        vgi_cmd_send_msg("Searching .elf (eboot):\n");
        for (int i = 0; i < 2; i++) {
            scan_memory(g_app_sce_info.segments[i].vaddr,
                        g_app_sce_info.segments[i].memsz,
                        patch_data);
        }
    }

    // Scan heap
    if (search_heap) {
        void *dummy = malloc(1);
        if (dummy == NULL) {
            vgi_cmd_send_msg("Could not malloc on heap, skipping...\n");
        } else {
            SceUID heap_memblock = sceKernelFindMemBlockByAddr(dummy, 0);
            void *heap_addr;
            SceKernelMemBlockInfo heap_info;
            heap_info.size = sizeof(SceKernelMemBlockInfo);
            free(dummy);

            if (sceKernelGetMemBlockBase(heap_memblock, &heap_addr) < 0
                    || sceKernelGetMemBlockInfoByAddr(heap_addr, &heap_info) < 0) {
                vgi_cmd_send_msg("Could not find heap base vaddr, skipping...\n");
            } else {
                snprintf(res_msg, RES_MSG_LEN, "Searching heap [0x%lX-0x%lX]:\n",
                        (uint32_t)heap_info.mappedBase,
                        (uint32_t)heap_info.mappedBase + heap_info.mappedSize);
                vgi_cmd_send_msg(res_msg);
                scan_memory(heap_info.mappedBase, heap_info.mappedSize, patch_data);
            }
        }
    }

    // Scan user memblocks
    if (search_blocks) {
        vgi_memblock_t *memblocks = vgi_get_memblocks();
        for (int i = 0; i < MAX_MEM_BLOCKS; i++) {
            if (!memblocks[i].active || memblocks[i].base == NULL)
                continue;

            snprintf(res_msg, RES_MSG_LEN, "Searching block '%s' (%s) [0x%lX-0x%lX]:\n",
                    memblocks[i].name,
                    vgi_get_memblock_type_text(memblocks[i].type),
                    (uint32_t)memblocks[i].base,
                    (uint32_t)memblocks[i].base + memblocks[i].size);
            vgi_cmd_send_msg(res_msg);
            scan_memory(memblocks[i].base, memblocks[i].size, patch_data);
        }
    }

    vgi_cmd_send_msg("Search done!\n");
}
