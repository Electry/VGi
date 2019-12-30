#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

typedef struct {
    SceGxmColorSurface *surface;
    uint8_t success;
    unsigned int width;
    unsigned int height;
} vgi_colorsurface_t;

static vgi_colorsurface_t g_color_surfaces[MAX_COLOR_SURFACES] = {0};
static int g_color_surfaces_count = 0;

static int sceGxmColorSurfaceInit_patched(
        SceGxmColorSurface *surface,
        SceGxmColorFormat colorFormat,
        SceGxmColorSurfaceType surfaceType,
        SceGxmColorSurfaceScaleMode scaleMode,
        SceGxmOutputRegisterSize outputRegisterSize,
        unsigned int width,
        unsigned int height,
        unsigned int strideInPixels,
        void *data
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_COLOR_SURFACE_INIT],
            surface, colorFormat, surfaceType, scaleMode, outputRegisterSize,
            width, height, strideInPixels, data);

    for (int i = 0; i < MAX_COLOR_SURFACES; i++) {
        if (g_color_surfaces[i].surface == surface || // Existing (reinit)
                !g_color_surfaces[i].surface) { // New
            if (!g_color_surfaces[i].surface)
                g_color_surfaces_count++;

            g_color_surfaces[i].surface = surface;
            g_color_surfaces[i].success = ret == SCE_OK;
            g_color_surfaces[i].width = width;
            g_color_surfaces[i].height = height;
            break;
        }
    }

    return ret;
}

static const char *get_cs_type_text(SceGxmColorSurfaceType type) {
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

void vgi_dump_graphics_cs() {
    char msg[128];
    snprintf(msg, sizeof(msg), "Color Surfaces (%d/%d):\n", g_color_surfaces_count, MAX_COLOR_SURFACES);
    vgi_cmd_send_msg(msg);

    for (int i = 0; i < MAX_COLOR_SURFACES; i++) {
        if (g_color_surfaces[i].surface) {
            unsigned int xMin, yMin, xMax, yMax;
            sceGxmColorSurfaceGetClip(g_color_surfaces[i].surface, &xMin, &yMin, &xMax, &yMax);

            snprintf(msg, sizeof(msg),
                    "  %4dx%-4d stride=%-4d MSAA=%-3s format=0x%-8X type=%-8s clipping=(%d, %d, %d, %d)%s\n",
                    g_color_surfaces[i].width,
                    g_color_surfaces[i].height,
                    sceGxmColorSurfaceGetStrideInPixels(g_color_surfaces[i].surface),
                    sceGxmColorSurfaceGetScaleMode(g_color_surfaces[i].surface)
                        == SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE ? "yes" : "no",
                    sceGxmColorSurfaceGetFormat(g_color_surfaces[i].surface),
                    get_cs_type_text(sceGxmColorSurfaceGetType(g_color_surfaces[i].surface)),
                    xMin,
                    yMin,
                    xMax,
                    yMax,
                    g_color_surfaces[i].success ? "" : " - FAIL!");
            vgi_cmd_send_msg(msg);
        }
    }
}

void vgi_draw_graphics_cs(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Color Surfaces (%d):", g_color_surfaces_count);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 10, 0), GUI_ANCHOR_TY(yoff, 1.5f),
            "   WxH    stride  MSAA  colorFormat   type      clipping");

    GUI_SCROLLABLE(MAX_COLOR_SURFACES, g_color_surfaces_count, 1,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_color_surfaces[i].surface) {
            if (g_color_surfaces[i].success)
                vgi_gui_set_text_color(255, 255, 255, 255);
            else
                vgi_gui_set_text_color(255, 0, 0, 255);

            unsigned int xMin, yMin, xMax, yMax;
            sceGxmColorSurfaceGetClip(g_color_surfaces[i].surface, &xMin, &yMin, &xMax, &yMax);

            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 3 + (i - g_scroll)),
                    "%4dx%-5d %-7d %-4s 0x%-10X %-9s (%d, %d, %d, %d)",
                    g_color_surfaces[i].width,
                    g_color_surfaces[i].height,
                    sceGxmColorSurfaceGetStrideInPixels(g_color_surfaces[i].surface),
                    sceGxmColorSurfaceGetScaleMode(g_color_surfaces[i].surface)
                        == SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE ? "DS" : "",
                    sceGxmColorSurfaceGetFormat(g_color_surfaces[i].surface),
                    get_cs_type_text(sceGxmColorSurfaceGetType(g_color_surfaces[i].surface)),
                    xMin,
                    yMin,
                    xMax,
                    yMax);
        }
    }
}

void vgi_setup_graphics_cs() {
    HOOK_FUNCTION_IMPORT(0xED0F6E25, HOOK_SCE_GXM_COLOR_SURFACE_INIT, sceGxmColorSurfaceInit_patched);
}
