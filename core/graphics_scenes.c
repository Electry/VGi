#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef struct {
    SceGxmContext *context;
    const SceGxmRenderTarget *renderTarget;
    const SceGxmColorSurface *colorSurface;
    const SceGxmDepthStencilSurface *depthStencilSurface;
    unsigned int xMax;
    unsigned int yMax;
} VGi_GxmScene;

static VGi_GxmScene g_gxmScenes[MAX_GXM_SCENES] = {0};
static uint32_t g_gxmScenesCount = 0;

static void addScene(
        SceGxmContext *immediateContext,
        const SceGxmRenderTarget *renderTarget,
        const SceGxmValidRegion *validRegion,
        const SceGxmColorSurface *colorSurface,
        const SceGxmDepthStencilSurface *depthStencilSurface) {

    for (int i = 0; i < MAX_GXM_SCENES; i++) {
         if ((g_gxmScenes[i].context == immediateContext &&
                g_gxmScenes[i].renderTarget == renderTarget &&
                g_gxmScenes[i].colorSurface == colorSurface &&
                g_gxmScenes[i].depthStencilSurface == depthStencilSurface &&
                (validRegion == NULL ||
                    (g_gxmScenes[i].xMax == validRegion->xMax &&
                    g_gxmScenes[i].yMax == validRegion->yMax)))) {
            break;
        }

        if (!g_gxmScenes[i].context) { // New
            g_gxmScenes[i].context = immediateContext;
            g_gxmScenes[i].renderTarget = renderTarget;
            g_gxmScenes[i].colorSurface = colorSurface;
            g_gxmScenes[i].depthStencilSurface = depthStencilSurface;
            if (validRegion != NULL) {
                g_gxmScenes[i].xMax = validRegion->xMax;
                g_gxmScenes[i].yMax = validRegion->yMax;
            } else {
                g_gxmScenes[i].xMax = 0;
                g_gxmScenes[i].yMax = 0;
            }
            g_gxmScenesCount++;
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
        const SceGxmDepthStencilSurface *depthStencilSurface) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_BEGIN_SCENE],
                    immediateContext, flags, renderTarget, validRegion, vertexSyncObject,
                    fragmentSyncObject, colorSurface, depthStencilSurface);
    if (ret != SCE_OK)
        return ret;

    addScene(immediateContext, renderTarget, validRegion, colorSurface, depthStencilSurface);
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
        const SceGxmDepthStencilSurface *storeDepthStencilSurface) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_BEGIN_SCENE_EX],
                    immediateContext, flags, renderTarget, validRegion, vertexSyncObject,
                    fragmentSyncObject, colorSurface, loadDepthStencilSurface, storeDepthStencilSurface);
    if (ret != SCE_OK)
        return ret;

    addScene(immediateContext, renderTarget, validRegion, colorSurface, loadDepthStencilSurface);
    return ret;
}



void drawGraphicsScenes(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Gxm Scenes (%d):", g_gxmScenesCount);
    osdDrawStringF(x += 20, y += osdGetLineHeight() * 1.5f,
                    "  context  renderTarget  colorSurface  dsSurface  validRegion");

    // Scrollable section
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    for (int i = g_scroll; i < MAX_GXM_SCENES; i++) {
        if (y > maxY - osdGetLineHeight()) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_gxmScenesCount - i);
            break;
        }

        if (g_gxmScenes[i].context) {
            osdDrawStringF(x, y += osdGetLineHeight(), "0x%08X  0x%-8X    0x%-8X   0x%-8X   %-4d %-4d",
                    g_gxmScenes[i].context,
                    g_gxmScenes[i].renderTarget,
                    g_gxmScenes[i].colorSurface,
                    g_gxmScenes[i].depthStencilSurface,
                    g_gxmScenes[i].xMax,
                    g_gxmScenes[i].yMax);
        }
    }
}

void setupGraphicsScenes() {
    hookFunctionImport(0x8734FF4E, HOOK_SCE_GXM_BEGIN_SCENE, sceGxmBeginScene_patched);
    hookFunctionImport(0x4709CF5A, HOOK_SCE_GXM_BEGIN_SCENE_EX, sceGxmBeginSceneEx_patched);
}
