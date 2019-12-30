#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

typedef struct {
    SceGxmContext *context;
    const SceGxmRenderTarget *render_target;
    const SceGxmColorSurface *color_surface;
    const SceGxmDepthStencilSurface *depth_stencil_surface;
    unsigned int x_max;
    unsigned int y_max;
} vgi_gxm_scene_t;

static vgi_gxm_scene_t g_gxm_scenes[MAX_GXM_SCENES] = {0};
static int g_gxm_scenes_count = 0;

static void add_scene(
        SceGxmContext *immediateContext,
        const SceGxmRenderTarget *renderTarget,
        const SceGxmValidRegion *validRegion,
        const SceGxmColorSurface *colorSurface,
        const SceGxmDepthStencilSurface *depthStencilSurface
) {
    for (int i = 0; i < MAX_GXM_SCENES; i++) {
         if ((g_gxm_scenes[i].context == immediateContext &&
                g_gxm_scenes[i].render_target == renderTarget &&
                g_gxm_scenes[i].color_surface == colorSurface &&
                g_gxm_scenes[i].depth_stencil_surface == depthStencilSurface &&
                (validRegion == NULL ||
                    (g_gxm_scenes[i].x_max == validRegion->xMax &&
                    g_gxm_scenes[i].y_max == validRegion->yMax)))) {
            break;
        }

        if (!g_gxm_scenes[i].context) { // New
            g_gxm_scenes[i].context = immediateContext;
            g_gxm_scenes[i].render_target = renderTarget;
            g_gxm_scenes[i].color_surface = colorSurface;
            g_gxm_scenes[i].depth_stencil_surface = depthStencilSurface;
            if (validRegion != NULL) {
                g_gxm_scenes[i].x_max = validRegion->xMax;
                g_gxm_scenes[i].y_max = validRegion->yMax;
            } else {
                g_gxm_scenes[i].x_max = 0;
                g_gxm_scenes[i].y_max = 0;
            }
            g_gxm_scenes_count++;
            break;
        }
    }
}

static int sceGxmBeginScene_patched(
        SceGxmContext *immediateContext,
        uint32_t flags,
        const SceGxmRenderTarget *renderTarget,
        const SceGxmValidRegion *validRegion,
        SceGxmSyncObject *vertexSyncObject,
        SceGxmSyncObject *fragmentSyncObject,
        const SceGxmColorSurface *colorSurface,
        const SceGxmDepthStencilSurface *depthStencilSurface
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_BEGIN_SCENE],
                            immediateContext, flags, renderTarget,
                            validRegion, vertexSyncObject, fragmentSyncObject,
                            colorSurface, depthStencilSurface);
    if (ret != SCE_OK)
        return ret;

    add_scene(immediateContext, renderTarget, validRegion, colorSurface, depthStencilSurface);
    return ret;
}

static int sceGxmBeginSceneEx_patched(
        SceGxmContext *immediateContext,
        uint32_t flags,
        const SceGxmRenderTarget *renderTarget,
        const SceGxmValidRegion *validRegion,
        SceGxmSyncObject *vertexSyncObject,
        SceGxmSyncObject *fragmentSyncObject,
        const SceGxmColorSurface *colorSurface,
        const SceGxmDepthStencilSurface *loadDepthStencilSurface,
        const SceGxmDepthStencilSurface *storeDepthStencilSurface
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_BEGIN_SCENE_EX],
                            immediateContext, flags, renderTarget,
                            validRegion, vertexSyncObject, fragmentSyncObject,
                            colorSurface, loadDepthStencilSurface, storeDepthStencilSurface);
    if (ret != SCE_OK)
        return ret;

    add_scene(immediateContext, renderTarget, validRegion, colorSurface, loadDepthStencilSurface);
    return ret;
}

void vgi_dump_graphics_scenes() {
    char msg[128];
    snprintf(msg, sizeof(msg), "Gxm Scenes (%d/%d):\n",
            g_gxm_scenes_count, MAX_GXM_SCENES);
    vgi_cmd_send_msg(msg);

    for (int i = 0; i < MAX_GXM_SCENES; i++) {
        if (g_gxm_scenes[i].context) {
            snprintf(msg, sizeof(msg),
                    "  0x%08lX rt=0x%-8lX cs=0x%-8lX  dss=0x%-8lX  x=%-4d y=%-4d\n",
                    (uint32_t)g_gxm_scenes[i].context,
                    (uint32_t)g_gxm_scenes[i].render_target,
                    (uint32_t)g_gxm_scenes[i].color_surface,
                    (uint32_t)g_gxm_scenes[i].depth_stencil_surface,
                    g_gxm_scenes[i].x_max,
                    g_gxm_scenes[i].y_max);
            vgi_cmd_send_msg(msg);
        }
    }
}

void vgi_draw_graphics_scenes(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Gxm Scenes (%d):", g_gxm_scenes_count);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 10, 0), GUI_ANCHOR_TY(yoff, 1.5f),
            "  context  renderTarget  colorSurface  dsSurface  validRegion");

    GUI_SCROLLABLE(MAX_GXM_SCENES, g_gxm_scenes_count, 1,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_gxm_scenes[i].context) {
            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 3 + (i - g_scroll)),
                    "0x%08X  0x%-8X    0x%-8X   0x%-8X   %-4d %-4d",
                    g_gxm_scenes[i].context,
                    g_gxm_scenes[i].render_target,
                    g_gxm_scenes[i].color_surface,
                    g_gxm_scenes[i].depth_stencil_surface,
                    g_gxm_scenes[i].x_max,
                    g_gxm_scenes[i].y_max);
        }
    }
}

void vgi_setup_graphics_scenes() {
    HOOK_FUNCTION_IMPORT(0x8734FF4E, HOOK_SCE_GXM_BEGIN_SCENE, sceGxmBeginScene_patched);
    HOOK_FUNCTION_IMPORT(0x4709CF5A, HOOK_SCE_GXM_BEGIN_SCENE_EX, sceGxmBeginSceneEx_patched);
}
