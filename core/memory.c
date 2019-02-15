#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef struct {
    uint8_t success;
    uint8_t active;
    char name[128];
    SceKernelMemBlockType type;
    int size;
    SceUID ret;
    void *base;
} VGi_MemBlock;

static VGi_MemBlock g_memBlocks[MAX_MEM_BLOCKS] = {0};
static uint32_t g_memBlocksCount = 0;

static SceUID sceKernelAllocMemBlock_patched(
        const char *name,
        SceKernelMemBlockType type,
        int size,
        void *optp) {
    SceUID ret = TAI_CONTINUE(SceUID, g_hookrefs[HOOK_SCE_KERNEL_ALLOC_MEM_BLOCK],
                                name, type, size, optp);

    for (int i = 0; i < MAX_MEM_BLOCKS; i++) {
        if (!g_memBlocks[i].active) {
            g_memBlocks[i].active = 1;
            g_memBlocks[i].success = ret >= SCE_OK;
            strncpy(g_memBlocks[i].name, name, 128);
            g_memBlocks[i].type = type;
            g_memBlocks[i].size = size;
            g_memBlocks[i].ret = ret;
            sceKernelGetMemBlockBase(ret, &(g_memBlocks[i].base));
            g_memBlocksCount++;
            break;
        }
    }

    return ret;
}

static int sceKernelFreeMemBlock_patched(SceUID uid) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_KERNEL_FREE_MEM_BLOCK], uid);
    if (ret < SCE_OK)
        return ret;

    for (int i = 0; i < MAX_MEM_BLOCKS; i++) {
        if (g_memBlocks[i].active && g_memBlocks[i].ret == uid) {
            g_memBlocks[i].active = 0;
            g_memBlocksCount--;
            break;
        }
    }

    return ret;
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
        snprintf(out, out_size, "%d %s", bytes / 1024 / 1024, "MB");
    } else if (bytes > 1024) {
        snprintf(out, out_size, "%d %s", bytes / 1024, "kB");
    } else {
        snprintf(out, out_size, "%d %s", bytes, "B");
    }
}

void drawMemory(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;

    SceKernelFreeMemorySizeInfo info;
    info.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&info);

    char buf[32], buf2[32], buf3[32];
    formatReadableSize(info.size_user, buf, 32);
    formatReadableSize(info.size_cdram, buf2, 32);
    formatReadableSize(info.size_phycont, buf3, 32);
    osdDrawStringF(x, y,  "Free RAM:  %s       VRAM: %s       PHYCONT: %s",
                            buf, buf2, buf3);

    osdDrawStringF(x, y += osdGetLineHeight() * 1.5f,
                    "Allocated MemBlocks (%d):", g_memBlocksCount);
    osdDrawStringF(x += 10, y += osdGetLineHeight() * 1.5f,
                    "  type          UID         base        size");

    // Scrollable section
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    for (int i = g_scroll; i < MAX_MEM_BLOCKS; i++) {
        if (y > maxY - osdGetLineHeight()*2) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_memBlocksCount - i);
            break;
        }

        if (g_memBlocks[i].active) {
            if (g_memBlocks[i].success)
                osdSetTextColor(255, 255, 255, 255);
            else
                osdSetTextColor(255, 0, 0, 255);

            formatReadableSize(g_memBlocks[i].size, buf, 32);
            osdDrawStringF(x, y += osdGetLineHeight(), "'%s'", g_memBlocks[i].name);
            osdDrawStringF(maxX - osdGetTextWidth(buf) - 40, y, "%s", buf);
            osdDrawStringF(x + 24, y += osdGetLineHeight(), "%-9s 0x%-10X 0x%-10X 0x%X B",
                    getMemBlockTypeString(g_memBlocks[i].type),
                    g_memBlocks[i].ret,
                    g_memBlocks[i].base,
                    g_memBlocks[i].size);
            y += 5;
        }
    }
}

void setupMemory() {
    hookFunctionImport(0xB9D5EBDE, HOOK_SCE_KERNEL_ALLOC_MEM_BLOCK, sceKernelAllocMemBlock_patched);
    hookFunctionImport(0xA91E15EE, HOOK_SCE_KERNEL_FREE_MEM_BLOCK, sceKernelFreeMemBlock_patched);
}
