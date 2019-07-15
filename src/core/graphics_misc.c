#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef struct {
    void *context;
    unsigned int xMin;
    unsigned int yMin;
    unsigned int xMax;
    unsigned int yMax;
} VGi_RegionClip;

typedef struct {
    void *context;
    float xOffset;
    float xScale;
    float yOffset;
    float yScale;
    float zOffset;
    float zScale;
} VGi_Viewport;

static VGi_RegionClip g_regionClips[MAX_REGION_CLIPS] = {0};
static uint32_t g_regionClipsCount = 0;

static VGi_Viewport g_viewports[MAX_VIEWPORTS] = {0};
static uint32_t g_viewportsCount = 0;

static void addRegionClip(
        SceGxmContext *context,
        unsigned int xMin,
        unsigned int yMin,
        unsigned int xMax,
        unsigned int yMax) {

    for (int i = 0; i < MAX_REGION_CLIPS; i++) {
        if (g_regionClips[i].context == context &&
                g_regionClips[i].xMin == xMin &&
                g_regionClips[i].yMin == yMin &&
                g_regionClips[i].xMax == xMax &&
                g_regionClips[i].yMax == yMax) {
            break; // Skip already known regions
        }

        if (!g_regionClips[i].context) {
            g_regionClipsCount++;
            g_regionClips[i].context = context;
            g_regionClips[i].xMin = xMin;
            g_regionClips[i].yMin = yMin;
            g_regionClips[i].xMax = xMax;
            g_regionClips[i].yMax = yMax;
            break;
        }
    }
}

static void sceGxmSetRegionClip_patched(
        SceGxmContext *context,
        SceGxmRegionClipMode mode,
        unsigned int xMin,
        unsigned int yMin,
        unsigned int xMax,
        unsigned int yMax) {

    TAI_CONTINUE(void, g_hookrefs[HOOK_SCE_GXM_SET_REGION_CLIP],
                    context, mode, xMin, yMin, xMax, yMax);

    addRegionClip(context, xMin, yMin, xMax, yMax);
}

static void sceGxmSetViewport_patched(
        SceGxmContext *context,
        float xOffset,
        float xScale,
        float yOffset,
        float yScale,
        float zOffset,
        float zScale) {
    
    // Haxxx
    uint32_t d32_1, d32_2, d32_3, d32_4, d32_5, d32_6;
    memcpy(&d32_1, &xOffset, sizeof(float));
    memcpy(&d32_2, &xScale, sizeof(float));
    memcpy(&d32_3, &yOffset, sizeof(float));
    memcpy(&d32_4, &yScale, sizeof(float));
    memcpy(&d32_5, &zOffset, sizeof(float));
    memcpy(&d32_6, &zScale, sizeof(float));

    TAI_CONTINUE(void, g_hookrefs[HOOK_SCE_GXM_SET_VIEWPORT],
                context, d32_1, d32_2, d32_3, d32_4, d32_5, d32_6);

    for (int i = 0; i < MAX_VIEWPORTS; i++) {
        if (g_viewports[i].context == context &&
                fabs(g_viewports[i].xOffset - xOffset) < 0.5f &&
                fabs(g_viewports[i].xScale - xScale) < 0.5f &&
                fabs(g_viewports[i].yOffset - yOffset) < 0.5f &&
                fabs(g_viewports[i].yScale - yScale) < 0.5f &&
                fabs(g_viewports[i].zOffset - zOffset) < 0.5f &&
                fabs(g_viewports[i].zScale - zScale) < 0.5f) {
            break; // Skip already known viewports
        }

        if (!g_viewports[i].context) {
            g_viewportsCount++;
            g_viewports[i].context = context;
            g_viewports[i].xOffset = xOffset;
            g_viewports[i].xScale = xScale;
            g_viewports[i].yOffset = yOffset;
            g_viewports[i].yScale = yScale;
            g_viewports[i].zOffset = zOffset;
            g_viewports[i].zScale = zScale;
            break;
        }
    }
}

static void sceGxmSetDefaultRegionClipAndViewport_patched(
        SceGxmContext *context,
        unsigned int xMax,
        unsigned int yMax) {
    TAI_CONTINUE(void, g_hookrefs[HOOK_SCE_GXM_SET_DEFAULT_REGION_CLIP_AND_VIEWPORT],
                    context, xMax, yMax);

    addRegionClip(context, 0, 0, xMax, yMax);
}

void drawGraphicsMisc(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Gxm Region Clips (%d):", g_regionClipsCount);

    // Scrollable section
    x += 20;
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    for (int i = g_scroll; i < MAX_REGION_CLIPS; i++) {
        if (y > minY + ((maxY - minY) / 2) - osdGetLineHeight()) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), minY + ((maxY - minY) / 2) - osdGetLineHeight() * 2,
                                1, g_regionClipsCount - i);
            break;
        }

        if (g_regionClips[i].context) {
            osdDrawStringF(x, y += osdGetLineHeight(), "0x%X (%d, %d, %d, %d)",
                    g_regionClips[i].context,
                    g_regionClips[i].xMin,
                    g_regionClips[i].yMin,
                    g_regionClips[i].xMax,
                    g_regionClips[i].yMax);
        }
    }

    y = minY + ((maxY - minY) / 2);
    osdDrawStringF(10, y, "Gxm Viewports (%d):", g_viewportsCount);
    y += osdGetLineHeight() * 0.5f;

    for (int i = g_scroll; i < MAX_VIEWPORTS; i++) {
        if (y > maxY - osdGetLineHeight()) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_viewportsCount - i);
            break;
        }

        if (g_viewports[i].context) {
            osdDrawStringF(x, y += 22, "0x%X (%.1f, %.1f, %.1f, %.1f, %.1f, %.1f)",
                    g_viewports[i].context,
                    g_viewports[i].xOffset,
                    g_viewports[i].xScale,
                    g_viewports[i].yOffset,
                    g_viewports[i].yScale,
                    g_viewports[i].zOffset,
                    g_viewports[i].zScale);
        }
    }
}

void setupGraphicsMisc() {
    hookFunctionImport(0x70C86868, HOOK_SCE_GXM_SET_REGION_CLIP, sceGxmSetRegionClip_patched);
    hookFunctionImport(0x3EB3380B, HOOK_SCE_GXM_SET_VIEWPORT, sceGxmSetViewport_patched);
    hookFunctionImport(0x60CF708E, HOOK_SCE_GXM_SET_DEFAULT_REGION_CLIP_AND_VIEWPORT, sceGxmSetDefaultRegionClipAndViewport_patched);
}
