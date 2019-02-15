#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef struct {
    SceUID id;
} VGi_Thread;

static VGi_Thread g_threads[MAX_THREADS] = {0};
static uint32_t g_threadsCount = 0;

static SceUID sceKernelCreateThread_patched(
        const char *name, SceKernelThreadEntry entry, int initPriority,
        int stackSize, SceUInt attr, int cpuAffinityMask,
        const SceKernelThreadOptParam *option) {
    SceUID ret = TAI_CONTINUE(SceUID, g_hookrefs[HOOK_SCE_KERNEL_CREATE_THREAD],
            name, entry, initPriority, stackSize, attr, cpuAffinityMask, option);
    if (ret < SCE_OK)
        return ret;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (!g_threads[i].id) {
            g_threads[i].id = ret;
            g_threadsCount++;
            break;
        }
    }

    return ret;
}

static int sceKernelDeleteThread_patched(SceUID thid) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_KERNEL_DELETE_THREAD], thid);
    if (ret < SCE_OK)
        return ret;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_threads[i].id == thid) {
            g_threads[i].id = 0;
            g_threadsCount--;
            break;
        }
    }

    return ret;
}

void drawThreads(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Threads (%d):", g_threadsCount);
    osdDrawStringF(x += 10, y += osdGetLineHeight() * 1.5f,
                    "    thID         entry        stack               priority affinity");

    // Scrollable section
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    SceKernelThreadInfo info;
    info.size = sizeof(SceKernelThreadInfo);

    for (int i = g_scroll; i < MAX_THREADS; i++) {
        if (y > maxY - osdGetLineHeight()*2) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_threadsCount - i);
            break;
        }

        if (g_threads[i].id && !sceKernelGetThreadInfo(g_threads[i].id, &info)) {
            osdDrawStringF(x, y += osdGetLineHeight(), "'%s' (%d)", info.name, info.status);
            osdDrawStringF(x + 24, y += osdGetLineHeight(), "0x%-10X 0x%-10X 0x%08X |0x%06X|   %-6d 0x%-10X",
                    g_threads[i].id,
                    info.entry,
                    info.stack,
                    info.stackSize,
                    info.currentPriority,
                    info.currentCpuAffinityMask);
            y += 5;
        }
    }
}

void setupThreads() {
    hookFunctionImport(0xC5C11EE7, HOOK_SCE_KERNEL_CREATE_THREAD, sceKernelCreateThread_patched);
    hookFunctionImport(0x1BBDE3D9, HOOK_SCE_KERNEL_DELETE_THREAD, sceKernelDeleteThread_patched);
}
