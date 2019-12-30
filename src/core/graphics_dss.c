#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

typedef struct {
    SceGxmDepthStencilSurface *surface;
    uint8_t success;
    SceGxmDepthStencilSurfaceType surface_type;
    unsigned int stride_in_samples;
    void *depth_data;
    void *stencil_data;
} vgi_depth_stencil_surface_t;

static vgi_depth_stencil_surface_t g_depth_stencil_surfaces[MAX_DEPTH_STENCIL_SURFACES] = {0};
static int g_depth_stencil_surfaces_count = 0;

static int sceGxmDepthStencilSurfaceInit_patched(
        SceGxmDepthStencilSurface *surface,
        SceGxmDepthStencilFormat depthStencilFormat,
        SceGxmDepthStencilSurfaceType surfaceType,
        unsigned int strideInSamples,
        void *depthData,
        void *stencilData
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_DEPTH_STENCIL_SURFACE_INIT],
                            surface, depthStencilFormat, surfaceType,
                            strideInSamples, depthData, stencilData);

    for (int i = 0; i < MAX_DEPTH_STENCIL_SURFACES; i++) {
        if (g_depth_stencil_surfaces[i].surface == surface || // Existing (reinit)
                !g_depth_stencil_surfaces[i].surface) { // New
            if (!g_depth_stencil_surfaces[i].surface)
                g_depth_stencil_surfaces_count++;

            g_depth_stencil_surfaces[i].surface = surface;
            g_depth_stencil_surfaces[i].success = ret == SCE_OK;
            g_depth_stencil_surfaces[i].surface_type = surfaceType;
            g_depth_stencil_surfaces[i].stride_in_samples = strideInSamples;
            g_depth_stencil_surfaces[i].depth_data = depthData;
            g_depth_stencil_surfaces[i].stencil_data = stencilData;
            break;
        }
    }

    return ret;
}

static const char *get_dss_type_text(SceGxmDepthStencilSurfaceType type) {
    switch (type) {
        case SCE_GXM_DEPTH_STENCIL_SURFACE_LINEAR:
            return "LINEAR";
        case SCE_GXM_DEPTH_STENCIL_SURFACE_TILED:
            return "TILED";
        default:
            return "?";
    }
}

void vgi_dump_graphics_dss() {
    char msg[128];
    snprintf(msg, sizeof(msg), "Depth/Stencil Surfaces (%d/%d):\n",
            g_depth_stencil_surfaces_count, MAX_DEPTH_STENCIL_SURFACES);
    vgi_cmd_send_msg(msg);

    for (int i = 0; i < MAX_DEPTH_STENCIL_SURFACES; i++) {
        if (g_depth_stencil_surfaces[i].surface) {
            snprintf(msg, sizeof(msg),
                    "  %-4d  surface=0x%-8lX  depth=0x%-8lX  stencil=0x%-8lX format=0x%-8X type=%s%s\n",
                    g_depth_stencil_surfaces[i].stride_in_samples,
                    (uint32_t)g_depth_stencil_surfaces[i].surface,
                    (uint32_t)g_depth_stencil_surfaces[i].depth_data,
                    (uint32_t)g_depth_stencil_surfaces[i].stencil_data,
                    sceGxmDepthStencilSurfaceGetFormat(g_depth_stencil_surfaces[i].surface),
                    get_dss_type_text(g_depth_stencil_surfaces[i].surface_type),
                    g_depth_stencil_surfaces[i].success ? "" : " - FAIL!");
            vgi_cmd_send_msg(msg);
        }
    }
}

void vgi_draw_graphics_dss(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Depth/Stencil Surfaces (%d):", g_depth_stencil_surfaces_count);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 10, 0), GUI_ANCHOR_TY(yoff, 1.5f),
            "stride  surface      depth       stencil     format     type");

    GUI_SCROLLABLE(MAX_DEPTH_STENCIL_SURFACES, g_depth_stencil_surfaces_count, 1,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_depth_stencil_surfaces[i].surface) {
            if (g_depth_stencil_surfaces[i].success)
                vgi_gui_set_text_color(255, 255, 255, 255);
            else
                vgi_gui_set_text_color(255, 0, 0, 255);

            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 3 + (i - g_scroll)),
                    " %-5d 0x%-9X 0x%-9X 0x%-9X 0x%-9X %s",
                    g_depth_stencil_surfaces[i].stride_in_samples,
                    g_depth_stencil_surfaces[i].surface,
                    g_depth_stencil_surfaces[i].depth_data,
                    g_depth_stencil_surfaces[i].stencil_data,
                    sceGxmDepthStencilSurfaceGetFormat(g_depth_stencil_surfaces[i].surface),
                    get_dss_type_text(g_depth_stencil_surfaces[i].surface_type));
        }
    }
}

void vgi_setup_graphics_dss() {
    HOOK_FUNCTION_IMPORT(0xCA9D41D1, HOOK_SCE_GXM_DEPTH_STENCIL_SURFACE_INIT, sceGxmDepthStencilSurfaceInit_patched);
}
