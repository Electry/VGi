#include <vitasdk.h>
#include <taihen.h>

#include "osd.h"
#include "main.h"

#define MAX_THREADS 64

typedef struct {
    SceUID id;
} VGi_Thread;

static VGi_Thread g_threads[MAX_THREADS] = {0};
static uint32_t g_threadsCount = 0;

static SceUID sceKernelCreateThread_patched(
        const char *name, SceKernelThreadEntry entry, int initPriority,
        int stackSize, SceUInt attr, int cpuAffinityMask,
        const SceKernelThreadOptParam *option) {
    int ret = TAI_CONTINUE(SceUID, g_hookrefs[33], name, entry,
            initPriority, stackSize, attr, cpuAffinityMask, option);

    for (int i = 0; i < MAX_THREADS; i++) {
        if (!g_threads[i].id) {
            g_threads[i].id = ret;
            goto RET;
        }
    }

RET:
    g_threadsCount++;
    return ret;
}

static int sceKernelDeleteThread_patched(SceUID thid) {

    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_threads[i].id == thid) {
            g_threads[i].id = 0;
            goto RET;
        }
    }

RET:
    g_threadsCount--;
    return TAI_CONTINUE(int, g_hookrefs[34], thid);
}

void drawThreadsMenu(const SceDisplayFrameBuf *pParam) {
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_THREADS) / 2, 5, MENU_TITLE_THREADS);

    osdSetTextScale(1);

    // Header
    osdDrawStringF(10, 60, "Threads (%d):", g_threadsCount);
    if (g_threadsCount > MAX_THREADS) {
        char buf[32];
        snprintf(buf, 32, "!! > %d", MAX_THREADS);
        osdDrawStringF(pParam->width - osdGetTextWidth(buf), 60, buf);
    }
    osdDrawStringF(30, 90, "    thid        entry        stack               priority  affinity");

    // Scrollable section
    int x = 30, y = 107;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        osdDrawStringF(pParam->width - 24 - 5, y + 22, "/\\");
        osdDrawStringF(pParam->width - 24 - 5, y + 44, "%2d", g_menuScroll);
    }

    SceKernelThreadInfo info;
    info.size = sizeof(SceKernelThreadInfo);

    for (int i = g_menuScroll; i < MAX_THREADS; i++) {
        if (g_threads[i].id && !sceKernelGetThreadInfo(g_threads[i].id, &info)) {
            osdDrawStringF(x, y += 27, "'%s' (%d)", info.name, info.status);
            osdDrawStringF(x + 24, y += 22, "0x%-10X 0x%-10X 0x%08X [0x%06X]  0x%-6X 0x%-10X",
                    g_threads[i].id,
                    info.entry,
                    info.stack,
                    info.stackSize,
                    info.currentPriority,
                    info.currentCpuAffinityMask);
        }

        // Do not draw out of screen
        if (y > pParam->height - 94) {
            // Draw scroll indicator
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 72, "%2d", MIN(g_threadsCount, MAX_THREADS) - i);
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 50, "\\/");
            break;
        }
    }
}

void setupThreadsMenu() {

    g_hooks[33] = taiHookFunctionImport(
                    &g_hookrefs[33],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xC5C11EE7,
                    sceKernelCreateThread_patched);

    g_hooks[34] = taiHookFunctionImport(
                    &g_hookrefs[34],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x1BBDE3D9,
                    sceKernelDeleteThread_patched);
}
