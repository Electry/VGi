#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef struct {
    uint8_t active;
    uint8_t success;
    SceGxmRenderTarget *ptr;
    SceGxmRenderTargetParams params;
    SceUID driverMemBlockBeforeCreate;
} VGi_RenderTarget;

static VGi_RenderTarget g_renderTargets[MAX_RENDER_TARGETS] = {0};
static uint32_t g_renderTargetsCount = 0;

static int sceGxmCreateRenderTarget_patched(
            const SceGxmRenderTargetParams *params,
            SceGxmRenderTarget *renderTarget) {
    SceUID dmb = params->driverMemBlock;
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_CREATE_RENDER_TARGET],
                            params, renderTarget);

    for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
        if (!g_renderTargets[i].active) {
            g_renderTargets[i].active = 1;
            g_renderTargets[i].success = ret == SCE_OK;
            g_renderTargets[i].ptr = renderTarget;
            g_renderTargets[i].driverMemBlockBeforeCreate = dmb;
            memcpy(&(g_renderTargets[i].params), params, sizeof(SceGxmRenderTargetParams));
            g_renderTargetsCount++;
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
        if (g_renderTargets[i].ptr == renderTarget && g_renderTargets[i].active) {
            g_renderTargets[i].active = 0;
             g_renderTargetsCount--;
             break;
        }
    }

    return ret;
}

void drawGraphicsRT(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Active Render Targets (%d):", g_renderTargetsCount);
    osdDrawStringF(x += 10, y += osdGetLineHeight() * 1.5f,
                        "   WxH         MSAA   scenesPF    memBlockUID");

    // Scrollable section
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    for (int i = g_scroll; i < MAX_RENDER_TARGETS; i++) {
        if (y > maxY - osdGetLineHeight()) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_renderTargetsCount - i);
            break;
        }

        if (g_renderTargets[i].active) {
            if (g_renderTargets[i].success)
                osdSetTextColor(255, 255, 255, 255);
            else
                osdSetTextColor(255, 0, 0, 255);

            osdDrawStringF(x, y += osdGetLineHeight(), "%4dx%-10d %-8s %-8d 0x%08X %s",
                    g_renderTargets[i].params.width,
                    g_renderTargets[i].params.height,
                    g_renderTargets[i].params.multisampleMode == SCE_GXM_MULTISAMPLE_4X ? "4x" :
                    (g_renderTargets[i].params.multisampleMode == SCE_GXM_MULTISAMPLE_2X ? "2x" : ""),
                    g_renderTargets[i].params.scenesPerFrame,
                    g_renderTargets[i].params.driverMemBlock,
                    g_renderTargets[i].driverMemBlockBeforeCreate == 0xFFFFFFFF ? "(A)" : "");
        }
    }
}

void setupGraphicsRT() {
    hookFunctionImport(0x207AF96B, HOOK_SCE_GXM_CREATE_RENDER_TARGET, sceGxmCreateRenderTarget_patched);
    hookFunctionImport(0x0B94C50A, HOOK_SCE_GXM_DESTROY_RENDER_TARGET, sceGxmDestroyRenderTarget_patched);
}
