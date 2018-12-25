#include <vitasdk.h>
#include <taihen.h>

#include "display.h"
#include "main.h"

#define MAX_RENDER_TARGETS          64
#define MAX_COLOR_SURFACES          64

typedef struct {
    uint8_t active;
    SceGxmRenderTarget *ptr;
    SceGxmRenderTargetParams params;
    SceUID driverMemBlockBeforeCreate;
} VGi_RenderTarget;

typedef struct {
    SceGxmColorSurface *surface;
    SceGxmColorFormat colorFormat;
    SceGxmColorSurfaceType surfaceType;
    SceGxmColorSurfaceScaleMode scaleMode;
    SceGxmOutputRegisterSize outputRegisterSize;
    unsigned int width;
    unsigned int height;
    unsigned int strideInPixels;
    void *data;
} VGi_ColorSurface;

static uint8_t sceDisplayGetRefreshRate_used = 0;
static uint8_t sceDisplayGetVcount_used = 0;
static uint8_t sceDisplayWaitVblankStart_used = 0;
static uint8_t sceDisplayWaitVblankStartCB_used = 0;
static uint8_t sceDisplayWaitVblankStartMulti_used = 0;
static uint8_t sceDisplayWaitVblankStartMultiCB_used = 0;
static uint8_t sceDisplayWaitSetFrameBuf_used = 0;
static uint8_t sceDisplayWaitSetFrameBufCB_used = 0;
static uint8_t sceDisplayWaitSetFrameBufMulti_used = 0;
static uint8_t sceDisplayWaitSetFrameBufMultiCB_used = 0;

static uint8_t sceDisplayWaitVblankStartMulti_vcount = 0;
static uint8_t sceDisplayWaitVblankStartMultiCB_vcount = 0;
static uint8_t sceDisplayWaitSetFrameBufMulti_vcount = 0;
static uint8_t sceDisplayWaitSetFrameBufMultiCB_vcount = 0;

static VGi_RenderTarget g_renderTargets[MAX_RENDER_TARGETS] = {0};
static uint32_t g_renderTargetsCount = 0;

static VGi_ColorSurface g_colorSurfaces[MAX_COLOR_SURFACES] = {0};
static uint32_t g_colorSurfacesCount = 0;

static SceGxmInitializeParams g_gxmInitializeParams = {0};

static int sceGxmCreateRenderTarget_patched(const SceGxmRenderTargetParams *params, SceGxmRenderTarget *renderTarget) {
    SceUID dmb = params->driverMemBlock;
    int ret = TAI_CONTINUE(int, g_hookrefs[11], params, renderTarget);

    for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
        if (!g_renderTargets[i].active) {
            g_renderTargets[i].active = 1;
            g_renderTargets[i].ptr = renderTarget;
            g_renderTargets[i].driverMemBlockBeforeCreate = dmb;
            memcpy(&(g_renderTargets[i].params), params, sizeof(SceGxmRenderTargetParams));
            goto RET;
        }
    }

RET:
    g_renderTargetsCount++;
    return ret;
}

static int sceGxmDestroyRenderTarget_patched(SceGxmRenderTarget *renderTarget) {
    for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
        if (g_renderTargets[i].ptr == renderTarget && g_renderTargets[i].active) {
            g_renderTargets[i].active = 0;
            goto RET;
        }
    }

RET:
    g_renderTargetsCount--;
    return TAI_CONTINUE(int, g_hookrefs[12], renderTarget);
}

static int sceDisplayGetRefreshRate_patched(float *pFps) {
    sceDisplayGetRefreshRate_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[1], pFps);
}
static int sceDisplayGetVcount_patched() {
    sceDisplayGetVcount_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[2]);
}
static int sceDisplayWaitVblankStart_patched() {
    sceDisplayWaitVblankStart_used = 1;
    if (g_vsyncKillswitch == 2)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[3]);
}
static int sceDisplayWaitVblankStartCB_patched() {
    sceDisplayWaitVblankStartCB_used = 1;
    if (g_vsyncKillswitch == 2)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[4]);
}
static int sceDisplayWaitVblankStartMulti_patched(unsigned int vcount) {
    sceDisplayWaitVblankStartMulti_used = 1;
    sceDisplayWaitVblankStartMulti_vcount = vcount;
    if (g_vsyncKillswitch == 2)
        return 0;
    if (g_vsyncKillswitch == 1)
        vcount = 1;
    return TAI_CONTINUE(int, g_hookrefs[5], vcount);
}
static int sceDisplayWaitVblankStartMultiCB_patched(unsigned int vcount) {
    sceDisplayWaitVblankStartMultiCB_used = 1;
    sceDisplayWaitVblankStartMultiCB_vcount = vcount;
    if (g_vsyncKillswitch == 2)
        return 0;
    if (g_vsyncKillswitch == 1)
        vcount = 1;
    return TAI_CONTINUE(int, g_hookrefs[6], vcount);
}
static int sceDisplayWaitSetFrameBuf_patched() {
    sceDisplayWaitSetFrameBuf_used = 1;
    if (g_vsyncKillswitch == 2)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[7]);
}
static int sceDisplayWaitSetFrameBufCB_patched() {
    sceDisplayWaitSetFrameBufCB_used = 1;
    if (g_vsyncKillswitch == 2)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[8]);
}
static int sceDisplayWaitSetFrameBufMulti_patched(unsigned int vcount) {
    sceDisplayWaitSetFrameBufMulti_used = 1;
    sceDisplayWaitSetFrameBufMulti_vcount = vcount;
    if (g_vsyncKillswitch == 2)
        return 0;
    if (g_vsyncKillswitch == 1)
        vcount = 1;
    return TAI_CONTINUE(int, g_hookrefs[9], vcount);
}
static int sceDisplayWaitSetFrameBufMultiCB_patched(unsigned int vcount) {
    sceDisplayWaitSetFrameBufMultiCB_used = 1;
    sceDisplayWaitSetFrameBufMultiCB_vcount = vcount;
    if (g_vsyncKillswitch == 2)
        return 0;
    if (g_vsyncKillswitch == 1)
        vcount = 1;
    return TAI_CONTINUE(int, g_hookrefs[10], vcount);
}

static int sceGxmColorSurfaceInit_patched(
        SceGxmColorSurface *surface,
        SceGxmColorFormat colorFormat,
        SceGxmColorSurfaceType surfaceType,
        SceGxmColorSurfaceScaleMode scaleMode,
        SceGxmOutputRegisterSize outputRegisterSize,
        unsigned int width,
        unsigned int height,
        unsigned int strideInPixels,
        void *data) {
    int ret = TAI_CONTINUE(int, g_hookrefs[22], surface, colorFormat, surfaceType,
            scaleMode, outputRegisterSize, width, height, strideInPixels, data);

    for (int i = 0; i < MAX_COLOR_SURFACES; i++) {
        if (!g_colorSurfaces[i].surface) {
            g_colorSurfaces[i].surface = surface;
            g_colorSurfaces[i].colorFormat = colorFormat;
            g_colorSurfaces[i].surfaceType = surfaceType;
            g_colorSurfaces[i].scaleMode = scaleMode;
            g_colorSurfaces[i].outputRegisterSize = outputRegisterSize;
            g_colorSurfaces[i].width = width;
            g_colorSurfaces[i].height = height;
            g_colorSurfaces[i].strideInPixels = strideInPixels;
            g_colorSurfaces[i].data = data;
            goto RET;
        }
    }

RET:
    g_colorSurfacesCount++;
    return ret;
}

static int sceGxmInitialize_patched(const SceGxmInitializeParams *params) {
    memcpy(&g_gxmInitializeParams, params, sizeof(SceGxmInitializeParams));
    return TAI_CONTINUE(int, g_hookrefs[25], params);
}

void drawGraphicsMenu(const SceDisplayFrameBuf *pParam) {
    setTextScale(2);
    drawStringF((pParam->width / 2) - getTextWidth(MENU_TITLE_GRAPHICS) / 2, 5, MENU_TITLE_GRAPHICS);

    setTextScale(1);
    drawStringF(0, 60,  "Framebuffer:        %dx%d, stride = %d", pParam->width, pParam->height, pParam->pitch);
    drawStringF(0, 82,  "Max Pending Swaps:  %d", g_gxmInitializeParams.displayQueueMaxPendingCount);
    drawStringF(0, 104, "DQ Callback:        0x%X", g_gxmInitializeParams.displayQueueCallback);
    drawStringF(0, 126, "Param. buf. size:   %d B", g_gxmInitializeParams.parameterBufferSize);

    drawStringF(0, 170, "VSync mechanisms:");
    if (!g_vsyncKillswitch) {
        setTextColor(150, 255, 150, 255);
        drawStringF(pParam->width - getTextWidth("DEFAULT (O)") - 10, 170, "DEFAULT (O)");
    } else if (g_vsyncKillswitch == 1) {
        setTextColor(255, 255, 150, 255);
        drawStringF(pParam->width - getTextWidth("ALLOW 1 (O)") - 10, 170, "ALLOW 1 (O)");
    } else if (g_vsyncKillswitch == 2) {
        setTextColor(255, 150, 150, 255);
        drawStringF(pParam->width - getTextWidth("DROP ALL (O)") - 10, 170, "DROP ALL (O)");
    }
    setTextColor(255, 255, 255, 255);

    int y = 192;
    if (sceDisplayGetRefreshRate_used)
        drawStringF(20, y += 22, "sceDisplayGetRefreshRate()");
    if (sceDisplayGetVcount_used)
        drawStringF(20, y += 22, "sceDisplayGetVcount()");
    if (sceDisplayWaitVblankStart_used)
        drawStringF(20, y += 22, "sceDisplayWaitVblankStart()");
    if (sceDisplayWaitVblankStartCB_used)
        drawStringF(20, y += 22, "sceDisplayWaitVblankStartCB()");
    if (sceDisplayWaitVblankStartMulti_used)
        drawStringF(20, y += 22, "sceDisplayWaitVblankStartMulti( %d )", sceDisplayWaitVblankStartMulti_vcount);
    if (sceDisplayWaitVblankStartMultiCB_used)
        drawStringF(20, y += 22, "sceDisplayWaitVblankStartMultiCB( %d )", sceDisplayWaitVblankStartMultiCB_vcount);
    if (sceDisplayWaitSetFrameBuf_used)
        drawStringF(20, y += 22, "sceDisplayWaitSetFrameBuf()");
    if (sceDisplayWaitSetFrameBufCB_used)
        drawStringF(20, y += 22, "sceDisplayWaitSetFrameBufCB()");
    if (sceDisplayWaitSetFrameBufMulti_used)
        drawStringF(20, y += 22, "sceDisplayWaitSetFrameBufMulti( %d )", sceDisplayWaitSetFrameBufMulti_vcount);
    if (sceDisplayWaitSetFrameBufMultiCB_used)
        drawStringF(20, y += 22, "sceDisplayWaitSetFrameBufMultiCB( %d )", sceDisplayWaitSetFrameBufMultiCB_vcount);
}

void drawGraphics2Menu(const SceDisplayFrameBuf *pParam) {
    setTextScale(2);
    drawStringF((pParam->width / 2) - getTextWidth(MENU_TITLE_GRAPHICS_2) / 2, 5, MENU_TITLE_GRAPHICS_2);

    // Header
    setTextScale(1);
    drawStringF(0, 60, "Active Render Targets (%d):", g_renderTargetsCount);
    if (g_renderTargetsCount > MAX_RENDER_TARGETS) {
        char buf[32];
        snprintf(buf, 32, "!! > %d", MAX_RENDER_TARGETS);
        drawStringF(pParam->width - getTextWidth(buf), 60, buf);
    }
    drawStringF(20, 93, "   WxH         MSAA   scenesPF    memBlockUID");

    // Scrollable section
    int x = 20, y = 104;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        drawStringF(pParam->width - 24, y + 22, "/\\");
        drawStringF(pParam->width - 24, y + 44, "%2d", g_menuScroll);
    }

    for (int i = g_menuScroll; i < MAX_RENDER_TARGETS; i++) {
        if (g_renderTargets[i].active) {
            drawStringF(x, y += 22, "%4dx%-10d %-8s %-8d 0x%08X %s",
                    g_renderTargets[i].params.width,
                    g_renderTargets[i].params.height,
                    g_renderTargets[i].params.multisampleMode == SCE_GXM_MULTISAMPLE_4X ? "4x" :
                    (g_renderTargets[i].params.multisampleMode == SCE_GXM_MULTISAMPLE_2X ? "2x" : ""),
                    g_renderTargets[i].params.scenesPerFrame,
                    g_renderTargets[i].params.driverMemBlock,
                    g_renderTargets[i].driverMemBlockBeforeCreate == 0xFFFFFFFF ? "(A)" : "");
        }

        // Do not draw out of screen
        if (y > pParam->height - 72) {
            // Draw scroll indicator
            drawStringF(pParam->width - 24, pParam->height - 72, "%2d", MIN(g_renderTargetsCount, MAX_RENDER_TARGETS) - i);
            drawStringF(pParam->width - 24, pParam->height - 50, "\\/");
            break;
        }
    }
}

void drawGraphics3Menu(const SceDisplayFrameBuf *pParam) {
    setTextScale(2);
    drawStringF((pParam->width / 2) - getTextWidth(MENU_TITLE_GRAPHICS_3) / 2, 5, MENU_TITLE_GRAPHICS_3);

    // Header
    setTextScale(1);
    drawStringF(0, 60, "Color Surfaces (%d):", g_colorSurfacesCount);
    if (g_colorSurfacesCount > MAX_COLOR_SURFACES) {
        char buf[32];
        snprintf(buf, 32, "!! > %d", MAX_COLOR_SURFACES);
        drawStringF(pParam->width - getTextWidth(buf), 60, buf);
    }
    drawStringF(20, 93, "   WxH      stride   MSAA   colorFormat  surfaceType");

    // Scrollable section
    int x = 20, y = 104;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        drawStringF(pParam->width - 24, y + 22, "/\\");
        drawStringF(pParam->width - 24, y + 44, "%2d", g_menuScroll);
    }

    for (int i = g_menuScroll; i < MAX_COLOR_SURFACES; i++) {
        if (g_colorSurfaces[i].surface) {
            drawStringF(x, y += 22, "%4dx%-7d %-8d %-6s 0x%-10X 0x%X",
                    g_colorSurfaces[i].width,
                    g_colorSurfaces[i].height,
                    g_colorSurfaces[i].strideInPixels,
                    g_colorSurfaces[i].scaleMode == SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE ? "DS" : "",
                    g_colorSurfaces[i].colorFormat,
                    g_colorSurfaces[i].surfaceType);
        }

        // Do not draw out of screen
        if (y > pParam->height - 72) {
            // Draw scroll indicator
            drawStringF(pParam->width - 24, pParam->height - 72, "%2d", MIN(g_colorSurfacesCount, MAX_COLOR_SURFACES) - i);
            drawStringF(pParam->width - 24, pParam->height - 50, "\\/");
            break;
        }
    }
}

void setupGraphicsMenu() {

    g_hooks[1] = taiHookFunctionImport(
                    &g_hookrefs[1],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xA08CA60D,
                    sceDisplayGetRefreshRate_patched);

    g_hooks[2] = taiHookFunctionImport(
                    &g_hookrefs[2],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xB6FDE0BA,
                    sceDisplayGetVcount_patched);

    g_hooks[3] = taiHookFunctionImport(
                    &g_hookrefs[3],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x5795E898,
                    sceDisplayWaitVblankStart_patched);

    g_hooks[4] = taiHookFunctionImport(
                    &g_hookrefs[4],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x78B41B92,
                    sceDisplayWaitVblankStartCB_patched);

    g_hooks[5] = taiHookFunctionImport(
                    &g_hookrefs[5],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xDD0A13B8,
                    sceDisplayWaitVblankStartMulti_patched);

    g_hooks[6] = taiHookFunctionImport(
                    &g_hookrefs[6],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x05F27764,
                    sceDisplayWaitVblankStartMultiCB_patched);

    g_hooks[7] = taiHookFunctionImport(
                    &g_hookrefs[7],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x9423560C,
                    sceDisplayWaitSetFrameBuf_patched);

    g_hooks[8] = taiHookFunctionImport(
                    &g_hookrefs[8],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x814C90AF,
                    sceDisplayWaitSetFrameBufCB_patched);

    g_hooks[9] = taiHookFunctionImport(
                    &g_hookrefs[9],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x7D9864A8,
                    sceDisplayWaitSetFrameBufMulti_patched);

    g_hooks[10] = taiHookFunctionImport(
                    &g_hookrefs[10],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x3E796EF5,
                    sceDisplayWaitSetFrameBufMultiCB_patched);

    g_hooks[11] = taiHookFunctionImport(
                    &g_hookrefs[11],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x207AF96B,
                    sceGxmCreateRenderTarget_patched);
    g_hooks[12] = taiHookFunctionImport(
                    &g_hookrefs[12],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x0B94C50A,
                    sceGxmDestroyRenderTarget_patched);

    g_hooks[22] = taiHookFunctionImport(
                    &g_hookrefs[22],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xED0F6E25,
                    sceGxmColorSurfaceInit_patched);

    g_hooks[25] = taiHookFunctionImport(
                    &g_hookrefs[25],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xB0F1E4EC,
                    sceGxmInitialize_patched);
}
