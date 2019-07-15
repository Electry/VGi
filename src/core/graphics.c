#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

static uint8_t g_sceDisplayGetRefreshRate_used = 0;
static uint8_t g_sceDisplayGetVcount_used = 0;
static uint8_t g_sceDisplayWaitVblankStart_used = 0;
static uint8_t g_sceDisplayWaitVblankStartCB_used = 0;
static uint8_t g_sceDisplayWaitVblankStartMulti_used = 0;
static uint8_t g_sceDisplayWaitVblankStartMultiCB_used = 0;
static uint8_t g_sceDisplayWaitSetFrameBuf_used = 0;
static uint8_t g_sceDisplayWaitSetFrameBufCB_used = 0;
static uint8_t g_sceDisplayWaitSetFrameBufMulti_used = 0;
static uint8_t g_sceDisplayWaitSetFrameBufMultiCB_used = 0;

static uint8_t g_sceDisplayWaitVblankStartMulti_vcount = 0;
static uint8_t g_sceDisplayWaitVblankStartMultiCB_vcount = 0;
static uint8_t g_sceDisplayWaitSetFrameBufMulti_vcount = 0;
static uint8_t g_sceDisplayWaitSetFrameBufMultiCB_vcount = 0;

static SceGxmInitializeParams g_gxmInitializeParams = {0};

#define VSYNC_DROP 3
static uint8_t g_vsyncKillswitch = 0; // 0 = off, x = force x, VSYNC_DROP = drop all

static int sceDisplayGetRefreshRate_patched(float *pFps) {
    g_sceDisplayGetRefreshRate_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_GET_REFRESH_RATE], pFps);
}
static int sceDisplayGetVcount_patched() {
    g_sceDisplayGetVcount_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_GET_VCOUNT]);
}
static int sceDisplayWaitVblankStart_patched() {
    if (g_vsyncKillswitch == VSYNC_DROP)
        return 0;
    g_sceDisplayWaitVblankStart_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_VBLANK_START]);
}
static int sceDisplayWaitVblankStartCB_patched() {
    if (g_vsyncKillswitch == VSYNC_DROP)
        return 0;
    g_sceDisplayWaitVblankStartCB_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_VBLANK_START_CB]);
}
static int sceDisplayWaitVblankStartMulti_patched(unsigned int vcount) {
    if (g_vsyncKillswitch == VSYNC_DROP)
        return 0;
    if (g_vsyncKillswitch)
        vcount = g_vsyncKillswitch;
    g_sceDisplayWaitVblankStartMulti_used = 1;
    g_sceDisplayWaitVblankStartMulti_vcount = vcount;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI], vcount);
}
static int sceDisplayWaitVblankStartMultiCB_patched(unsigned int vcount) {
    if (g_vsyncKillswitch == VSYNC_DROP)
        return 0;
    if (g_vsyncKillswitch)
        vcount = g_vsyncKillswitch;
    g_sceDisplayWaitVblankStartMultiCB_used = 1;
    g_sceDisplayWaitVblankStartMultiCB_vcount = vcount;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI_CB], vcount);
}
static int sceDisplayWaitSetFrameBuf_patched() {
    if (g_vsyncKillswitch == VSYNC_DROP)
        return 0;
    g_sceDisplayWaitSetFrameBuf_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF]);
}
static int sceDisplayWaitSetFrameBufCB_patched() {
    if (g_vsyncKillswitch == VSYNC_DROP)
        return 0;
    g_sceDisplayWaitSetFrameBufCB_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_CB]);
}
static int sceDisplayWaitSetFrameBufMulti_patched(unsigned int vcount) {
    if (g_vsyncKillswitch == VSYNC_DROP)
        return 0;
    if (g_vsyncKillswitch)
        vcount = g_vsyncKillswitch;
    g_sceDisplayWaitSetFrameBufMulti_used = 1;
    g_sceDisplayWaitSetFrameBufMulti_vcount = vcount;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI], vcount);
}
static int sceDisplayWaitSetFrameBufMultiCB_patched(unsigned int vcount) {
    if (g_vsyncKillswitch == VSYNC_DROP)
        return 0;
    if (g_vsyncKillswitch)
        vcount = g_vsyncKillswitch;
    g_sceDisplayWaitSetFrameBufMultiCB_used = 1;
    g_sceDisplayWaitSetFrameBufMultiCB_vcount = vcount;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI_CB], vcount);
}

static int sceGxmInitialize_patched(const SceGxmInitializeParams *params) {
    memcpy(&g_gxmInitializeParams, params, sizeof(SceGxmInitializeParams));
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_INITIALIZE], params);
}

static void drawGraphicsVsyncItem(
        int *x, int *y,
        const char *str,
        uint8_t *item, uint8_t *vcount) {
    if (!*item)
        return; // Never called

    if (*item == 1) // Called since last setFrameBuf
        osdSetTextColor(255, 255, 255, 255);
    else // Called sometime before
        osdSetTextColor(155, 155, 155, 255);

    *y += osdGetLineHeight();
    if (vcount != NULL) {
        osdDrawStringF(*x, *y, str, *vcount);
    } else {
        osdDrawStringF(*x, *y, str);
    }

    // Reset usage
    *item = 2;
}

void checkButtonsGraphics(uint32_t buttons_now, uint32_t buttons) {
    // Vsync Killswitch
    if (buttons & SCE_CTRL_CIRCLE) {
        // No "multi" func is used, skip multi opts
        if (!g_sceDisplayWaitVblankStartMulti_used &&
                !g_sceDisplayWaitVblankStartMultiCB_used &&
                !g_sceDisplayWaitSetFrameBufMulti_used &&
                !g_sceDisplayWaitSetFrameBufMultiCB_used) {
            g_vsyncKillswitch = !g_vsyncKillswitch ? VSYNC_DROP : 0;
        }
        // otherwise
        else {
            if (g_vsyncKillswitch < VSYNC_DROP)
                g_vsyncKillswitch++;
            else
                g_vsyncKillswitch = 0;
        }
    }
}

void drawGraphics(int minX, int minY, int maxX, int maxY, const SceDisplayFrameBuf *pParam) {
    int x = minX, y = minY;
    osdDrawStringF(x, y,                        "Framebuffer:        %dx%d, stride = %d", pParam->width, pParam->height, pParam->pitch);
    osdDrawStringF(x, y += osdGetLineHeight(),  "Max Pending Swaps:  %d", g_gxmInitializeParams.displayQueueMaxPendingCount);
    osdDrawStringF(x, y += osdGetLineHeight(),  "DQ Callback:        0x%X", g_gxmInitializeParams.displayQueueCallback);
    osdDrawStringF(x, y += osdGetLineHeight(),  "Param. buf. size:   %d B = 0x%X", g_gxmInitializeParams.parameterBufferSize,
                                                            g_gxmInitializeParams.parameterBufferSize);

    y += osdGetLineHeight();
    osdDrawStringF(x, y += osdGetLineHeight(), "VSync:");
    if (!g_vsyncKillswitch) {
        osdSetTextColor(150, 255, 150, 255);
        osdDrawStringF(maxX - osdGetTextWidth("DEFAULT (O)") - 10, y, "DEFAULT (O)");
    } else if (g_vsyncKillswitch == 3) {
        osdSetTextColor(255, 150, 150, 255);
        osdDrawStringF(maxX - osdGetTextWidth("DROP ALL (O)") - 10, y, "DROP ALL (O)");
    } else {
        osdSetTextColor(255, 255, 150, 255);
        osdDrawStringF(maxX - osdGetTextWidth("FORCE x (O)") - 10, y, "FORCE %d (O)", g_vsyncKillswitch);
    }
    osdSetTextColor(255, 255, 255, 255);

    x += 20;
    drawGraphicsVsyncItem(&x, &y, "sceDisplayGetRefreshRate()",
                            &g_sceDisplayGetRefreshRate_used, NULL);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayGetVcount()",
                            &g_sceDisplayGetVcount_used, NULL);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayWaitVblankStart()",
                            &g_sceDisplayWaitVblankStart_used, NULL);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayWaitVblankStartCB()",
                            &g_sceDisplayWaitVblankStartCB_used, NULL);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayWaitVblankStartMulti( %d )",
                            &g_sceDisplayWaitVblankStartMulti_used, &g_sceDisplayWaitVblankStartMulti_vcount);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayWaitVblankStartMultiCB( %d )",
                            &g_sceDisplayWaitVblankStartMultiCB_used, &g_sceDisplayWaitVblankStartMultiCB_vcount);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayWaitSetFrameBuf()",
                            &g_sceDisplayWaitSetFrameBuf_used, NULL);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayWaitSetFrameBufCB()",
                            &g_sceDisplayWaitSetFrameBufCB_used, NULL);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayWaitSetFrameBufMulti( %d )",
                            &g_sceDisplayWaitSetFrameBufMulti_used, &g_sceDisplayWaitSetFrameBufMulti_vcount);
    drawGraphicsVsyncItem(&x, &y, "sceDisplayWaitSetFrameBufMultiCB( %d )",
                            &g_sceDisplayWaitSetFrameBufMultiCB_used, &g_sceDisplayWaitSetFrameBufMultiCB_vcount);
}

void setupGraphics() {
    hookFunctionImport(0xA08CA60D, HOOK_SCE_DISPLAY_GET_REFRESH_RATE, sceDisplayGetRefreshRate_patched);
    hookFunctionImport(0xB6FDE0BA, HOOK_SCE_DISPLAY_GET_VCOUNT, sceDisplayGetVcount_patched);
    hookFunctionImport(0x5795E898, HOOK_SCE_DISPLAY_WAIT_VBLANK_START, sceDisplayWaitVblankStart_patched);
    hookFunctionImport(0x78B41B92, HOOK_SCE_DISPLAY_WAIT_VBLANK_START_CB, sceDisplayWaitVblankStartCB_patched);
    hookFunctionImport(0xDD0A13B8, HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI, sceDisplayWaitVblankStartMulti_patched);
    hookFunctionImport(0x05F27764, HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI_CB, sceDisplayWaitVblankStartMultiCB_patched);
    hookFunctionImport(0x9423560C, HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF, sceDisplayWaitSetFrameBuf_patched);
    hookFunctionImport(0x814C90AF, HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_CB, sceDisplayWaitSetFrameBufCB_patched);
    hookFunctionImport(0x7D9864A8, HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI, sceDisplayWaitSetFrameBufMulti_patched);
    hookFunctionImport(0x3E796EF5, HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI_CB, sceDisplayWaitSetFrameBufMultiCB_patched);
    hookFunctionImport(0xB0F1E4EC, HOOK_SCE_GXM_INITIALIZE, sceGxmInitialize_patched);
}
