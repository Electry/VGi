#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>

#include "../osd.h"
#include "../main.h"

struct mem_callback_table1 {
    int unk0;
    int unk4;
    int (*user_malloc_init)();
    int (*user_malloc_finalize)();
    void *(*user_malloc)(int);
    void (*user_free)(void*);
    int (*user_calloc)(int,int);
    int (*user_realloc)(int,int);
    int (*user_memalign)(int,int);
    int (*user_reallocalign)(int,int,int);
    int (*user_malloc_stats)(int);
    int (*user_malloc_stats_fast)(int);
    int (*user_malloc_usable_size)(int);
};

struct local_8708FC60_ctx_holder {
    char pad0[8 * 4];
    struct mem_callback_table1 *callback_table1;
    char pad1[5 * 4];
};

struct process_param {
    char pad0[11 * 4];
    struct local_8708FC60_ctx_holder *hldr_2C; // SceLibcparam
};

struct malloc_managed_size {
  size_t max_system_size;
  size_t current_system_size;
  size_t max_inuse_size;
  size_t current_inuse_size;
  char pad0[4 * 4];
};

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
static SceKernelFreeMemorySizeInfo g_startFreeMem = {0};

static bool g_hasCustomHeapImpl = false;
static uint32_t g_customHeapInitFnPtr = 0;

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
        snprintf(out, out_size, "%d%s", bytes / 1024 / 1024, "MB");
    } else if (bytes > 1024) {
        snprintf(out, out_size, "%d%s", bytes / 1024, "kB");
    } else {
        snprintf(out, out_size, "%d%s", bytes, "B");
    }
}

void drawMemory(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;

    SceKernelFreeMemorySizeInfo info;
    info.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&info);

    char buf[16], buf2[16], buf3[16];
    char buf4[16], buf5[16], buf6[16];
    formatReadableSize(info.size_user, buf, 16);
    formatReadableSize(info.size_cdram, buf2, 16);
    formatReadableSize(info.size_phycont, buf3, 16);
    formatReadableSize(g_startFreeMem.size_user, buf4, 16);
    formatReadableSize(g_startFreeMem.size_cdram, buf5, 16);
    formatReadableSize(g_startFreeMem.size_phycont, buf6, 16);
    osdDrawStringF(x, y,  "Free RAM:  %s / %s    VRAM: %s / %s    PHYCONT: %s / %s",
                            buf, buf4, buf2, buf5, buf3, buf6);

    struct malloc_managed_size ms;
    malloc_stats_fast(&ms);
    formatReadableSize(ms.current_system_size - ms.current_inuse_size, buf, 16);
    formatReadableSize(ms.current_system_size, buf2, 16);
    formatReadableSize(ms.max_system_size, buf3, 16);
    osdDrawStringF(x, y += osdGetLineHeight(),  "Free heap: %s / %s (max. %s)",
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

    y += osdGetLineHeight();
    if (y <= maxY - osdGetLineHeight()*2) {
        // Print warning
        if (g_hasCustomHeapImpl) {
            osdDrawStringF(x, y += osdGetLineHeight(),  "Target is using custom heap impl.!");
            osdDrawStringF(x, y += osdGetLineHeight(),  "Mem. blocks allocated in user_malloc_init()==0x%X are not shown!",
                    g_customHeapInitFnPtr);
        }
    }
}

void setupMemory() {
    // poll free mem
    g_startFreeMem.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&g_startFreeMem);

    // is using custom malloc?
    struct process_param *pparam = (struct process_param *)sceKernelGetProcessParam(); // 0x2BE3E066
    if (pparam
            && pparam->hldr_2C
            && pparam->hldr_2C->callback_table1
            && pparam->hldr_2C->callback_table1->user_malloc_init) {
        g_hasCustomHeapImpl = true;
        g_customHeapInitFnPtr = (uint32_t)pparam->hldr_2C->callback_table1->user_malloc_init;
    }

    hookFunctionImport(0xB9D5EBDE, HOOK_SCE_KERNEL_ALLOC_MEM_BLOCK, sceKernelAllocMemBlock_patched);
    hookFunctionImport(0xA91E15EE, HOOK_SCE_KERNEL_FREE_MEM_BLOCK, sceKernelFreeMemBlock_patched);
}
