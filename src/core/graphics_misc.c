#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

typedef struct {
    void *context;
    unsigned int x_min;
    unsigned int y_min;
    unsigned int x_max;
    unsigned int y_max;
} vgi_region_clip_t;

typedef struct {
    void *context;
    float x_offset;
    float x_scale;
    float y_offset;
    float y_scale;
    float z_offset;
    float z_scale;
} vgi_viewport_t;

static vgi_region_clip_t g_region_clips[MAX_REGION_CLIPS] = {0};
static int g_region_clips_count = 0;

static vgi_viewport_t g_viewports[MAX_VIEWPORTS] = {0};
static int g_viewports_count = 0;

static void add_region_clip(
        SceGxmContext *context,
        unsigned int xMin,
        unsigned int yMin,
        unsigned int xMax,
        unsigned int yMax
) {
    for (int i = 0; i < MAX_REGION_CLIPS; i++) {
        if (g_region_clips[i].context == context &&
                g_region_clips[i].x_min == xMin &&
                g_region_clips[i].y_min == yMin &&
                g_region_clips[i].x_max == xMax &&
                g_region_clips[i].y_max == yMax) {
            break; // Skip already known regions
        }

        if (!g_region_clips[i].context) {
            g_region_clips_count++;
            g_region_clips[i].context = context;
            g_region_clips[i].x_min = xMin;
            g_region_clips[i].y_min = yMin;
            g_region_clips[i].x_max = xMax;
            g_region_clips[i].y_max = yMax;
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
        unsigned int yMax
) {
    TAI_CONTINUE(void, g_hookrefs[HOOK_SCE_GXM_SET_REGION_CLIP],
                    context, mode, xMin, yMin, xMax, yMax);

    add_region_clip(context, xMin, yMin, xMax, yMax);
}

static void sceGxmSetViewport_patched(
        SceGxmContext *context,
        float xOffset, float xScale,
        float yOffset, float yScale,
        float zOffset, float zScale
) {
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
                fabs(g_viewports[i].x_offset - xOffset) < 0.5f &&
                fabs(g_viewports[i].x_scale - xScale) < 0.5f &&
                fabs(g_viewports[i].y_offset - yOffset) < 0.5f &&
                fabs(g_viewports[i].y_scale - yScale) < 0.5f &&
                fabs(g_viewports[i].z_offset - zOffset) < 0.5f &&
                fabs(g_viewports[i].z_scale - zScale) < 0.5f) {
            break; // Skip already known viewports
        }

        if (!g_viewports[i].context) {
            g_viewports_count++;
            g_viewports[i].context = context;
            g_viewports[i].x_offset = xOffset;
            g_viewports[i].x_scale = xScale;
            g_viewports[i].y_offset = yOffset;
            g_viewports[i].y_scale = yScale;
            g_viewports[i].z_offset = zOffset;
            g_viewports[i].z_scale = zScale;
            break;
        }
    }
}

static void sceGxmSetDefaultRegionClipAndViewport_patched(
        SceGxmContext *context,
        unsigned int xMax,
        unsigned int yMax
) {
    TAI_CONTINUE(void, g_hookrefs[HOOK_SCE_GXM_SET_DEFAULT_REGION_CLIP_AND_VIEWPORT],
                    context, xMax, yMax);

    add_region_clip(context, 0, 0, xMax, yMax);
}

void vgi_dump_graphics_misc() {
    char msg[128];
    snprintf(msg, sizeof(msg), "Region Clips (%d/%d):\n",
            g_region_clips_count, MAX_REGION_CLIPS);
    vgi_cmd_send_msg(msg);

    for (int i = 0; i < MAX_REGION_CLIPS; i++) {
        if (g_region_clips[i].context) {
            snprintf(msg, sizeof(msg),
                    "  0x%lX (%d, %d, %d, %d)\n",
                    (uint32_t)g_region_clips[i].context,
                    g_region_clips[i].x_min,
                    g_region_clips[i].y_min,
                    g_region_clips[i].x_max,
                    g_region_clips[i].y_max);
            vgi_cmd_send_msg(msg);
        }
    }

    snprintf(msg, sizeof(msg), "Viewports (%d/%d):\n",
            g_viewports_count, MAX_VIEWPORTS);
    vgi_cmd_send_msg(msg);

    for (int i = 0; i < MAX_VIEWPORTS; i++) {
        if (g_viewports[i].context) {
            snprintf(msg, sizeof(msg),
                    "  0x%lX (%.1f, %.1f, %.1f, %.1f, %.1f, %.1f)\n",
                    (uint32_t)g_viewports[i].context,
                    g_viewports[i].x_offset,
                    g_viewports[i].x_scale,
                    g_viewports[i].y_offset,
                    g_viewports[i].y_scale,
                    g_viewports[i].z_offset,
                    g_viewports[i].z_scale);
            vgi_cmd_send_msg(msg);
        }
    }
}

void vgi_draw_graphics_misc(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Region Clips (%d):", g_region_clips_count);

    GUI_SCROLLABLE(MAX_REGION_CLIPS, g_region_clips_count, 1,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 1.5f),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_CY(0) - GUI_FONT_H) {
        if (g_region_clips[i].context) {
            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 1.5f + (i - g_scroll)),
                    "0x%X (%d, %d, %d, %d)",
                    g_region_clips[i].context,
                    g_region_clips[i].x_min,
                    g_region_clips[i].y_min,
                    g_region_clips[i].x_max,
                    g_region_clips[i].y_max);
        }
    }

    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_CY(0),
            "Viewports (%d):", g_viewports_count);

    GUI_SCROLLABLE(MAX_VIEWPORTS, g_viewports_count, 1,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_CY(0) + GUI_FONT_H,
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_viewports[i].context) {
            vgi_gui_printf(
                    GUI_ANCHOR_LX(xoff, 1),
                    GUI_ANCHOR_CY(0) + GUI_ANCHOR_TY(0, 1.5f + (i - g_scroll)),
                    "0x%X (%.1f, %.1f, %.1f, %.1f, %.1f, %.1f)",
                    g_viewports[i].context,
                    g_viewports[i].x_offset,
                    g_viewports[i].x_scale,
                    g_viewports[i].y_offset,
                    g_viewports[i].y_scale,
                    g_viewports[i].z_offset,
                    g_viewports[i].z_scale);
        }
    }
}

void vgi_setup_graphics_misc() {
    HOOK_FUNCTION_IMPORT(0x70C86868, HOOK_SCE_GXM_SET_REGION_CLIP, sceGxmSetRegionClip_patched);
    HOOK_FUNCTION_IMPORT(0x3EB3380B, HOOK_SCE_GXM_SET_VIEWPORT, sceGxmSetViewport_patched);
    HOOK_FUNCTION_IMPORT(0x60CF708E, HOOK_SCE_GXM_SET_DEFAULT_REGION_CLIP_AND_VIEWPORT, sceGxmSetDefaultRegionClipAndViewport_patched);
}
