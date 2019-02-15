#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef struct {
    SceGxmColorSurface *surface;
    uint8_t success;
    unsigned int width;
    unsigned int height;
} VGi_ColorSurface;

static VGi_ColorSurface g_colorSurfaces[MAX_COLOR_SURFACES] = {0};
static uint32_t g_colorSurfacesCount = 0;

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
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_COLOR_SURFACE_INIT],
            surface, colorFormat, surfaceType, scaleMode, outputRegisterSize,
            width, height, strideInPixels, data);

    for (int i = 0; i < MAX_COLOR_SURFACES; i++) {
        if (g_colorSurfaces[i].surface == surface || // Existing (reinit)
                !g_colorSurfaces[i].surface) { // New
            if (!g_colorSurfaces[i].surface)
                g_colorSurfacesCount++;

            g_colorSurfaces[i].surface = surface;
            g_colorSurfaces[i].success = ret == SCE_OK;
            g_colorSurfaces[i].width = width;
            g_colorSurfaces[i].height = height;
            break;
        }
    }

    return ret;
}

static const char *getCSTypeString(SceGxmColorSurfaceType type) {
    switch (type) {
        case SCE_GXM_COLOR_SURFACE_LINEAR:
            return "LINEAR";
        case SCE_GXM_COLOR_SURFACE_TILED:
            return "TILED";
        case SCE_GXM_COLOR_SURFACE_SWIZZLED:
            return "SWIZZLED";
        default:
            return "?";
    }
}

void drawGraphicsCS(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Color Surfaces (%d):", g_colorSurfacesCount);
    osdDrawStringF(x += 10, y += osdGetLineHeight() * 1.5f,
                    "   WxH    stride  MSAA  colorFormat   type      clipping");

    // Scrollable section
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    for (int i = g_scroll; i < MAX_COLOR_SURFACES; i++) {
        if (y > maxY - osdGetLineHeight()) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_colorSurfacesCount - i);
            break;
        }

        if (g_colorSurfaces[i].surface) {
            if (g_colorSurfaces[i].success)
                osdSetTextColor(255, 255, 255, 255);
            else
                osdSetTextColor(255, 0, 0, 255);

            unsigned int xMin, yMin, xMax, yMax;
            sceGxmColorSurfaceGetClip(g_colorSurfaces[i].surface, &xMin, &yMin, &xMax, &yMax);

            osdDrawStringF(x, y += osdGetLineHeight(), "%4dx%-5d %-7d %-4s 0x%-10X %-9s (%d, %d, %d, %d)",
                    g_colorSurfaces[i].width,
                    g_colorSurfaces[i].height,
                    sceGxmColorSurfaceGetStrideInPixels(g_colorSurfaces[i].surface),
                    sceGxmColorSurfaceGetScaleMode(g_colorSurfaces[i].surface) == SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE ? "DS" : "",                    
                    sceGxmColorSurfaceGetFormat(g_colorSurfaces[i].surface),
                    getCSTypeString(sceGxmColorSurfaceGetType(g_colorSurfaces[i].surface)),
                    xMin,
                    yMin,
                    xMax,
                    yMax);
        }
    }
}

void setupGraphicsCS() {
    hookFunctionImport(0xED0F6E25, HOOK_SCE_GXM_COLOR_SURFACE_INIT, sceGxmColorSurfaceInit_patched);
}
