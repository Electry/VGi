#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"

typedef struct {
    SceUID id;
} vgi_thread_t;

static vgi_thread_t g_threads[MAX_THREADS] = {0};
static uint32_t g_threads_count = 0;

static SceUID sceKernelCreateThread_patched(
        const char *name,
        SceKernelThreadEntry entry,
        int initPriority,
        int stackSize,
        SceUInt attr,
        int cpuAffinityMask,
        const SceKernelThreadOptParam *option
) {
    SceUID ret = TAI_CONTINUE(SceUID, g_hookrefs[HOOK_SCE_KERNEL_CREATE_THREAD],
                                name, entry, initPriority, stackSize, attr,
                                cpuAffinityMask, option);
    if (ret < SCE_OK)
        return ret;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (!g_threads[i].id) {
            g_threads[i].id = ret;
            g_threads_count++;
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
            g_threads_count--;
            break;
        }
    }

    return ret;
}

void vgi_draw_threads(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Threads (%d):", g_threads_count);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 10, 0), GUI_ANCHOR_TY(yoff, 1.5f),
            "    thID         entry        stack               priority affinity");

    SceKernelThreadInfo info;
    info.size = sizeof(SceKernelThreadInfo);

    GUI_SCROLLABLE(MAX_THREADS, g_threads_count, 2,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_threads[i].id && !sceKernelGetThreadInfo(g_threads[i].id, &info)) {
            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 3 + 2 * (i - g_scroll)),
                    "'%s' (%d)", info.name, info.status);
            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 4 + 2 * (i - g_scroll)),
                    "0x%-10X 0x%-10X 0x%08X |0x%06X|   %-6d 0x%-10X",
                    g_threads[i].id,
                    info.entry,
                    info.stack,
                    info.stackSize,
                    info.currentPriority,
                    info.currentCpuAffinityMask);
        }
    }
}

void vgi_setup_threads() {
    HOOK_FUNCTION_IMPORT(0xC5C11EE7, HOOK_SCE_KERNEL_CREATE_THREAD, sceKernelCreateThread_patched);
    HOOK_FUNCTION_IMPORT(0x1BBDE3D9, HOOK_SCE_KERNEL_DELETE_THREAD, sceKernelDeleteThread_patched);
}
