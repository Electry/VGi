#include <libk/stdio.h>
#include <vitasdk.h>
#include <taihen.h>

#include "display.h"
#include "main.h"

#define MAX_MEM_BLOCKS 64

typedef struct {
    uint8_t active;
    char name[128];
    SceKernelMemBlockType type;
    int size;
    void *optp;
    SceUID ret;
} VGi_MemBlock;

static VGi_MemBlock g_memBlocks[MAX_MEM_BLOCKS] = {0};
static uint32_t g_memBlocksCount = 0;

static SceUID sceKernelAllocMemBlock_patched(const char *name, SceKernelMemBlockType type, int size, void *optp) {
    int ret = TAI_CONTINUE(SceUID, g_hookrefs[23], name, type, size, optp);

    for (int i = 0; i < MAX_MEM_BLOCKS; i++) {
        if (!g_memBlocks[i].active) {
            g_memBlocks[i].active = 1;
            strncpy(g_memBlocks[i].name, name, 128);
            g_memBlocks[i].type = type;
            g_memBlocks[i].size = size;
            g_memBlocks[i].ret = ret;
            goto RET;
        }
    }

RET:
    g_memBlocksCount++;
    return ret;
}

static int sceKernelFreeMemBlock_patched(SceUID uid) {

    for (int i = 0; i < MAX_MEM_BLOCKS; i++) {
        if (g_memBlocks[i].active && g_memBlocks[i].ret == uid) {
            g_memBlocks[i].active = 0;
            goto RET;
        }
    }

RET:
    g_memBlocksCount--;
    return TAI_CONTINUE(int, g_hookrefs[24], uid);
}


const char *getMemBlockTypeString(SceKernelMemBlockType type) {
    switch (type) {
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE:
            return "UNCACHE";
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_RW:
            return "RW";
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW:
            return "PHYCONT";
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW:
            return "PHYCONT_NC";
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW:
            return "CDRAM";
        default:
            return "?";
    }
}

void formatReadableSize(int bytes, char *out, size_t out_size) {
    if (bytes > 1024*1024) {
        snprintf(out, out_size, "%3d %s", bytes / 1024 / 1024, "MB");
    } else if (bytes > 1024) {
        snprintf(out, out_size, "%3d %s", bytes / 1024, "kB");
    } else {
        snprintf(out, out_size, "%3d %s", bytes, "B");
    }
}

void drawMemoryMenu(const SceDisplayFrameBuf *pParam) {
    setTextScale(2);
    drawStringF((pParam->width / 2) - getTextWidth(MENU_TITLE_MEMORY) / 2, 5, MENU_TITLE_MEMORY);

    setTextScale(1);

    SceKernelFreeMemorySizeInfo info;
    info.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&info);

    char buf[32];
    formatReadableSize(info.size_user, buf, 32);
    drawStringF(0, 60,  "Free RAM:     %s", buf);
    formatReadableSize(info.size_cdram, buf, 32);
    drawStringF(0, 82,  "Free VRAM:    %s", buf);
    formatReadableSize(info.size_phycont, buf, 32);
    drawStringF(0, 104, "Free phycont: %s", buf);

    drawStringF(0, 148, "Allocated MemBlocks (%d):", g_memBlocksCount);
    int x = 20, y = 170;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        drawStringF(pParam->width - 24, y + 22, "/\\");
        drawStringF(pParam->width - 24, y + 44, "%2d", g_menuScroll);
    }

    for (int i = g_menuScroll; i < MAX_MEM_BLOCKS; i++) {
        // Draw only active blocks
        if (g_memBlocks[i].active) {
            formatReadableSize(g_memBlocks[i].size, buf, 32);
            drawStringF(x, y += 22, "%s", g_memBlocks[i].name);
            drawStringF(pParam->width - getTextWidth(buf) - 40, y, "%s", buf);
            drawStringF(x + 20, y += 22, "size = %d, type = %s, uid = 0x%08X",
                    g_memBlocks[i].size,
                    getMemBlockTypeString(g_memBlocks[i].type),
                    g_memBlocks[i].ret);
        }

        // Do not draw out of screen
        if (y > pParam->height - 72) {
            // Draw scroll indicator
            drawStringF(pParam->width - 24, pParam->height - 72, "%2d", g_memBlocksCount - i);
            drawStringF(pParam->width - 24, pParam->height - 50, "\\/");
            break;
        }
    }
}

void setupMemoryMenu() {

    g_hooks[23] = taiHookFunctionImport(
                    &g_hookrefs[23],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xB9D5EBDE,
                    sceKernelAllocMemBlock_patched);

    g_hooks[24] = taiHookFunctionImport(
                    &g_hookrefs[24],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xA91E15EE,
                    sceKernelFreeMemBlock_patched);
}
