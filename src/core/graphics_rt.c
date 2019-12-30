#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

typedef struct {
    uint8_t active;
    uint8_t success;
    SceGxmRenderTarget *ptr;
    SceGxmRenderTargetParams params;
    SceUID driver_memblock_before_create;
} vgi_render_target_t;

static vgi_render_target_t g_render_targets[MAX_RENDER_TARGETS] = {0};
static int g_render_targets_count = 0;

static int sceGxmCreateRenderTarget_patched(
        const SceGxmRenderTargetParams *params,
        SceGxmRenderTarget *renderTarget
) {
    SceUID dmb = params->driverMemBlock;
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_CREATE_RENDER_TARGET],
                            params, renderTarget);

    for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
        if (!g_render_targets[i].active) {
            g_render_targets[i].active = 1;
            g_render_targets[i].success = ret == SCE_OK;
            g_render_targets[i].ptr = renderTarget;
            g_render_targets[i].driver_memblock_before_create = dmb;
            memcpy(&(g_render_targets[i].params), params, sizeof(SceGxmRenderTargetParams));
            g_render_targets_count++;
            break;
        }
    }

    return ret;
}

static int sceGxmDestroyRenderTarget_patched(SceGxmRenderTarget *renderTarget) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_DESTROY_RENDER_TARGET], renderTarget);
    if (ret != SCE_OK)
        return ret;

    for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
        if (g_render_targets[i].ptr == renderTarget && g_render_targets[i].active) {
            g_render_targets[i].active = 0;
            g_render_targets_count--;
            break;
        }
    }

    return ret;
}

void vgi_dump_graphics_rt() {
    char msg[128];
    snprintf(msg, sizeof(msg), "Active Render Targets (%d/%d):\n",
            g_render_targets_count, MAX_RENDER_TARGETS);
    vgi_cmd_send_msg(msg);

    for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
        if (g_render_targets[i].active) {
            snprintf(msg, sizeof(msg),
                    "  %4dx%-4d MSAA=%-2s spf=%d mem_uid=0x%08X %s%s\n",
                    g_render_targets[i].params.width,
                    g_render_targets[i].params.height,
                    g_render_targets[i].params.multisampleMode == SCE_GXM_MULTISAMPLE_4X ? "4x" :
                    (g_render_targets[i].params.multisampleMode == SCE_GXM_MULTISAMPLE_2X ? "2x" : "no"),
                    g_render_targets[i].params.scenesPerFrame,
                    g_render_targets[i].params.driverMemBlock,
                    g_render_targets[i].driver_memblock_before_create == 0xFFFFFFFF ? "(A)" : "",
                    g_render_targets[i].success ? "" : " - FAIL!");
            vgi_cmd_send_msg(msg);
        }
    }
}

void vgi_draw_graphics_rt(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Active Render Targets (%d):", g_render_targets_count);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 10, 0), GUI_ANCHOR_TY(yoff, 1.5f),
            "   WxH         MSAA   scenesPF    memBlockUID");

    GUI_SCROLLABLE(MAX_RENDER_TARGETS, g_render_targets_count, 1,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_render_targets[i].active) {
            if (g_render_targets[i].success)
                vgi_gui_set_text_color(255, 255, 255, 255);
            else
                vgi_gui_set_text_color(255, 0, 0, 255);

            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 3 + (i - g_scroll)),
                    "%4dx%-10d %-8s %-8d 0x%08X %s",
                    g_render_targets[i].params.width,
                    g_render_targets[i].params.height,
                    g_render_targets[i].params.multisampleMode == SCE_GXM_MULTISAMPLE_4X ? "4x" :
                    (g_render_targets[i].params.multisampleMode == SCE_GXM_MULTISAMPLE_2X ? "2x" : ""),
                    g_render_targets[i].params.scenesPerFrame,
                    g_render_targets[i].params.driverMemBlock,
                    g_render_targets[i].driver_memblock_before_create == 0xFFFFFFFF ? "(A)" : "");
        }
    }
}

void vgi_setup_graphics_rt() {
    HOOK_FUNCTION_IMPORT(0x207AF96B, HOOK_SCE_GXM_CREATE_RENDER_TARGET, sceGxmCreateRenderTarget_patched);
    HOOK_FUNCTION_IMPORT(0x0B94C50A, HOOK_SCE_GXM_DESTROY_RENDER_TARGET, sceGxmDestroyRenderTarget_patched);
}
