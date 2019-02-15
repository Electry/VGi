#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef struct {
    SceGxmDepthStencilSurface *surface;
    uint8_t success;
    SceGxmDepthStencilSurfaceType surfaceType;
    unsigned int strideInSamples;
    void *depthData;
    void *stencilData;
} VGi_DepthStencilSurface;

static VGi_DepthStencilSurface g_depthStencilSurfaces[MAX_DEPTH_STENCIL_SURFACES] = {0};
static uint32_t g_depthStencilSurfacesCount = 0;

static int sceGxmDepthStencilSurfaceInit_patched(
        SceGxmDepthStencilSurface *surface,
        SceGxmDepthStencilFormat depthStencilFormat,
        SceGxmDepthStencilSurfaceType surfaceType,
        unsigned int strideInSamples,
        void *depthData,
        void *stencilData) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_DEPTH_STENCIL_SURFACE_INIT],
                            surface, depthStencilFormat, surfaceType,
                            strideInSamples, depthData, stencilData);

    for (int i = 0; i < MAX_DEPTH_STENCIL_SURFACES; i++) {
        if (g_depthStencilSurfaces[i].surface == surface || // Existing (reinit)
                !g_depthStencilSurfaces[i].surface) { // New
            if (!g_depthStencilSurfaces[i].surface)
                g_depthStencilSurfacesCount++;

            g_depthStencilSurfaces[i].surface = surface;
            g_depthStencilSurfaces[i].success = ret == SCE_OK;
            g_depthStencilSurfaces[i].surfaceType = surfaceType;
            g_depthStencilSurfaces[i].strideInSamples = strideInSamples;
            g_depthStencilSurfaces[i].depthData = depthData;
            g_depthStencilSurfaces[i].stencilData = stencilData;
            break;
        }
    }

    return ret;
}

static const char *getDSSTypeString(SceGxmDepthStencilSurfaceType type) {
    switch (type) {
        case SCE_GXM_DEPTH_STENCIL_SURFACE_LINEAR:
            return "LINEAR";
        case SCE_GXM_DEPTH_STENCIL_SURFACE_TILED:
            return "TILED";
        default:
            return "?";
    }
}

void drawGraphicsDSS(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Depth/Stencil Surfaces (%d):", g_depthStencilSurfacesCount);
    osdDrawStringF(x += 10, y += osdGetLineHeight() * 1.5f,
                    "stride  surface      depth       stencil     format     type");

    // Scrollable section
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    for (int i = g_scroll; i < MAX_DEPTH_STENCIL_SURFACES; i++) {
        if (y > maxY - osdGetLineHeight()) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_depthStencilSurfacesCount - i);
            break;
        }

        if (g_depthStencilSurfaces[i].surface) {
            if (g_depthStencilSurfaces[i].success)
                osdSetTextColor(255, 255, 255, 255);
            else
                osdSetTextColor(255, 0, 0, 255);

            osdDrawStringF(x, y += osdGetLineHeight(), " %-5d 0x%-9X 0x%-9X 0x%-9X 0x%-9X %s",
                    g_depthStencilSurfaces[i].strideInSamples,
                    g_depthStencilSurfaces[i].surface,
                    g_depthStencilSurfaces[i].depthData,
                    g_depthStencilSurfaces[i].stencilData,
                    sceGxmDepthStencilSurfaceGetFormat(g_depthStencilSurfaces[i].surface),
                    getDSSTypeString(g_depthStencilSurfaces[i].surfaceType));
        }
    }
}

void setupGraphicsDSS() {
    hookFunctionImport(0xCA9D41D1, HOOK_SCE_GXM_DEPTH_STENCIL_SURFACE_INIT, sceGxmDepthStencilSurfaceInit_patched);
}
