#include <vitasdk.h>
#include <taihen.h>

#include "osd.h"
#include "main.h"

#define MAX_MEM_BLOCKS 64

typedef struct {
    uint8_t active;
    char name[128];
    SceKernelMemBlockType type;
    int size;
    void *optp;
    SceUID ret;
    void *base;
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
            sceKernelGetMemBlockBase(ret, &(g_memBlocks[i].base));
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
            return "PHY_NC";
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
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_MEMORY) / 2, 5, MENU_TITLE_MEMORY);

    osdSetTextScale(1);

    SceKernelFreeMemorySizeInfo info;
    info.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&info);

    char buf[32];
    formatReadableSize(info.size_user, buf, 32);
    osdDrawStringF(0, 60,  "Free RAM:     %s", buf);
    formatReadableSize(info.size_cdram, buf, 32);
    osdDrawStringF(0, 82,  "Free VRAM:    %s", buf);
    formatReadableSize(info.size_phycont, buf, 32);
    osdDrawStringF(0, 104, "Free phycont: %s", buf);

    // Header
    osdDrawStringF(pParam->width - 388, 104, "Allocated MemBlocks (%d):", g_memBlocksCount);
    if (g_memBlocksCount > MAX_MEM_BLOCKS) {
        char buf[32];
        snprintf(buf, 32, "!! > %d", MAX_MEM_BLOCKS);
        osdDrawStringF(pParam->width - osdGetTextWidth(buf), 104, buf);
    }
    osdDrawStringF(20, 148, "  type         UID          base      size");

    // Scrollable section
    int x = 20, y = 165;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        osdDrawStringF(pParam->width - 24, y + 22, "/\\");
        osdDrawStringF(pParam->width - 24, y + 44, "%2d", g_menuScroll);
    }

    for (int i = g_menuScroll; i < MAX_MEM_BLOCKS; i++) {
        // Draw only active blocks
        if (g_memBlocks[i].active) {
            formatReadableSize(g_memBlocks[i].size, buf, 32);
            osdDrawStringF(x, y += 27, "'%s'", g_memBlocks[i].name);
            osdDrawStringF(pParam->width - osdGetTextWidth(buf) - 40, y, "%s", buf);
            osdDrawStringF(x + 24, y += 22, "%-9s 0x%-10X 0x%-10X %d B",
                    getMemBlockTypeString(g_memBlocks[i].type),
                    g_memBlocks[i].ret,
                    g_memBlocks[i].base,
                    g_memBlocks[i].size);
        }

        // Do not draw out of screen
        if (y > pParam->height - 94) {
            // Draw scroll indicator
            osdDrawStringF(pParam->width - 24, pParam->height - 72, "%2d", MIN(g_memBlocksCount, MAX_MEM_BLOCKS) - i);
            osdDrawStringF(pParam->width - 24, pParam->height - 50, "\\/");
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
