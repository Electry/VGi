#include <vitasdk.h>
#include <taihen.h>

#include "osd.h"
#include "main.h"

#define MAX_RENDER_TARGETS          64
#define MAX_COLOR_SURFACES          64
#define MAX_DEPTH_STENCIL_SURFACES  64
#define MAX_TEXTURES                64

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

typedef struct {
    SceGxmDepthStencilSurface *surface;
    SceGxmDepthStencilFormat depthStencilFormat;
    SceGxmDepthStencilSurfaceType surfaceType;
    unsigned int strideInSamples;
    void *depthData;
    void *stencilData;
} VGi_DepthStencilSurface;

typedef enum {
    TEXTURE_LINEAR,
    TEXTURE_LINEAR_STRIDED,
    TEXTURE_SWIZZLED,
    TEXTURE_TILED,
    TEXTURE_CUBE
} VGi_TextureType;

typedef struct {
    SceGxmTexture *texture;
    VGi_TextureType type;
    const void *data;
    SceGxmTextureFormat texFormat;
    unsigned int width;
    unsigned int height;
    unsigned int mipCount;
    unsigned int byteStride;
} VGi_Texture;

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

static VGi_DepthStencilSurface g_depthStencilSurfaces[MAX_DEPTH_STENCIL_SURFACES] = {0};
static uint32_t g_depthStencilSurfacesCount = 0;

static VGi_Texture g_textures[MAX_TEXTURES] = {0};
static uint32_t g_texturesCount = 0;

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

    int i = 0;
    for (i = 0; i < MAX_COLOR_SURFACES; i++) {
        if (g_colorSurfaces[i].surface == surface) {
            goto RET_EXISTING;
        }
    }
    for (i = 0; i < MAX_COLOR_SURFACES; i++) {
        if (!g_colorSurfaces[i].surface) {
            goto RET_NEW;
        }
    }

    return ret;

RET_NEW:
    g_colorSurfacesCount++;
RET_EXISTING:
    g_colorSurfaces[i].surface = surface;
    g_colorSurfaces[i].colorFormat = colorFormat;
    g_colorSurfaces[i].surfaceType = surfaceType;
    g_colorSurfaces[i].scaleMode = scaleMode;
    g_colorSurfaces[i].outputRegisterSize = outputRegisterSize;
    g_colorSurfaces[i].width = width;
    g_colorSurfaces[i].height = height;
    g_colorSurfaces[i].strideInPixels = strideInPixels;
    g_colorSurfaces[i].data = data;
    return ret;
}

static int sceGxmInitialize_patched(const SceGxmInitializeParams *params) {
    memcpy(&g_gxmInitializeParams, params, sizeof(SceGxmInitializeParams));
    return TAI_CONTINUE(int, g_hookrefs[25], params);
}

static int sceGxmDepthStencilSurfaceInit_patched(
        SceGxmDepthStencilSurface *surface,
        SceGxmDepthStencilFormat depthStencilFormat,
        SceGxmDepthStencilSurfaceType surfaceType,
        unsigned int strideInSamples,
        void *depthData,
        void *stencilData) {
    int ret = TAI_CONTINUE(int, g_hookrefs[27], surface, depthStencilFormat, surfaceType,
            strideInSamples, depthData, stencilData);

    int i = 0;
    for (i = 0; i < MAX_DEPTH_STENCIL_SURFACES; i++) {
        if (g_depthStencilSurfaces[i].surface == surface) {
            goto RET_EXISTING;
        }
    }
    for (i = 0; i < MAX_DEPTH_STENCIL_SURFACES; i++) {
        if (!g_depthStencilSurfaces[i].surface) {
            goto RET_NEW;
        }
    }

    return ret;

RET_NEW:
    g_depthStencilSurfacesCount++;
RET_EXISTING:
    g_depthStencilSurfaces[i].surface = surface;
    g_depthStencilSurfaces[i].depthStencilFormat = depthStencilFormat;
    g_depthStencilSurfaces[i].surfaceType = surfaceType;
    g_depthStencilSurfaces[i].strideInSamples = strideInSamples;
    g_depthStencilSurfaces[i].depthData = depthData;
    g_depthStencilSurfaces[i].stencilData = stencilData;
    return ret;
}

static void addTexture(
        VGi_TextureType type,
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount,
        unsigned int byteStride) {
    int i = 0;
    for (i = 0; i < MAX_TEXTURES; i++) {
        if (g_textures[i].texture == texture) {
            goto RET_EXISTING;
        }
    }
    for (i = 0; i < MAX_TEXTURES; i++) {
        if (!g_textures[i].texture) {
            goto RET_NEW;
        }
    }

RET_NEW:
    g_texturesCount++;
RET_EXISTING:
    g_textures[i].texture = texture;
    g_textures[i].type = type;
    g_textures[i].data = data;
    g_textures[i].texFormat = texFormat;
    g_textures[i].width = width;
    g_textures[i].height = height;
    g_textures[i].mipCount = mipCount;
    g_textures[i].byteStride = byteStride;
}

static int sceGxmTextureInitSwizzled_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount) {
    int ret = TAI_CONTINUE(int, g_hookrefs[28], texture, data, texFormat,
            width, height, mipCount);
    addTexture(TEXTURE_SWIZZLED, texture, data, texFormat, width, height, mipCount, 0);
    return ret;
}
static int sceGxmTextureInitLinear_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount) {
    int ret = TAI_CONTINUE(int, g_hookrefs[29], texture, data, texFormat,
            width, height, mipCount);
    addTexture(TEXTURE_LINEAR, texture, data, texFormat, width, height, mipCount, 0);
    return ret;
}
static int sceGxmTextureInitLinearStrided_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int byteStride) {
    int ret = TAI_CONTINUE(int, g_hookrefs[30], texture, data, texFormat,
            width, height, byteStride);
    addTexture(TEXTURE_LINEAR_STRIDED, texture, data, texFormat, width, height, 0, byteStride);
    return ret;
}
static int sceGxmTextureInitTiled_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount) {
    int ret = TAI_CONTINUE(int, g_hookrefs[31], texture, data, texFormat,
            width, height, mipCount);
    addTexture(TEXTURE_TILED, texture, data, texFormat, width, height, mipCount, 0);
    return ret;
}
static int sceGxmTextureInitCube_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount) {
    int ret = TAI_CONTINUE(int, g_hookrefs[32], texture, data, texFormat,
            width, height, mipCount);
    addTexture(TEXTURE_CUBE, texture, data, texFormat, width, height, mipCount, 0);
    return ret;
}

static const char *getTextureTypeString(VGi_TextureType type) {
    switch (type) {
        case TEXTURE_LINEAR:
            return "LINEAR";
        case TEXTURE_LINEAR_STRIDED:
            return "LINEAR_S";
        case TEXTURE_SWIZZLED:
            return "SWIZZLED";
        case TEXTURE_TILED:
            return "TILED";
        case TEXTURE_CUBE:
            return "CUBE";
        default:
            return "?";
    }
}

void drawGraphicsMenu(const SceDisplayFrameBuf *pParam) {
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_GRAPHICS) / 2, 5, MENU_TITLE_GRAPHICS);

    osdSetTextScale(1);
    osdDrawStringF(10, 60,  "Framebuffer:        %dx%d, stride = %d", pParam->width, pParam->height, pParam->pitch);
    osdDrawStringF(10, 82,  "Max Pending Swaps:  %d", g_gxmInitializeParams.displayQueueMaxPendingCount);
    osdDrawStringF(10, 104, "DQ Callback:        0x%X", g_gxmInitializeParams.displayQueueCallback);
    osdDrawStringF(10, 126, "Param. buf. size:   %d B", g_gxmInitializeParams.parameterBufferSize);

    osdDrawStringF(10, 170, "VSync mechanisms:");
    if (!g_vsyncKillswitch) {
        osdSetTextColor(150, 255, 150, 255);
        osdDrawStringF(pParam->width - osdGetTextWidth("DEFAULT (O)") - 10, 170, "DEFAULT (O)");
    } else if (g_vsyncKillswitch == 1) {
        osdSetTextColor(255, 255, 150, 255);
        osdDrawStringF(pParam->width - osdGetTextWidth("ALLOW 1 (O)") - 10, 170, "ALLOW 1 (O)");
    } else if (g_vsyncKillswitch == 2) {
        osdSetTextColor(255, 150, 150, 255);
        osdDrawStringF(pParam->width - osdGetTextWidth("DROP ALL (O)") - 10, 170, "DROP ALL (O)");
    }
    osdSetTextColor(255, 255, 255, 255);

    int x = 30, y = 192;
    if (sceDisplayGetRefreshRate_used)
        osdDrawStringF(x, y += 22, "sceDisplayGetRefreshRate()");
    if (sceDisplayGetVcount_used)
        osdDrawStringF(x, y += 22, "sceDisplayGetVcount()");
    if (sceDisplayWaitVblankStart_used)
        osdDrawStringF(x, y += 22, "sceDisplayWaitVblankStart()");
    if (sceDisplayWaitVblankStartCB_used)
        osdDrawStringF(x, y += 22, "sceDisplayWaitVblankStartCB()");
    if (sceDisplayWaitVblankStartMulti_used)
        osdDrawStringF(x, y += 22, "sceDisplayWaitVblankStartMulti( %d )", sceDisplayWaitVblankStartMulti_vcount);
    if (sceDisplayWaitVblankStartMultiCB_used)
        osdDrawStringF(x, y += 22, "sceDisplayWaitVblankStartMultiCB( %d )", sceDisplayWaitVblankStartMultiCB_vcount);
    if (sceDisplayWaitSetFrameBuf_used)
        osdDrawStringF(x, y += 22, "sceDisplayWaitSetFrameBuf()");
    if (sceDisplayWaitSetFrameBufCB_used)
        osdDrawStringF(x, y += 22, "sceDisplayWaitSetFrameBufCB()");
    if (sceDisplayWaitSetFrameBufMulti_used)
        osdDrawStringF(x, y += 22, "sceDisplayWaitSetFrameBufMulti( %d )", sceDisplayWaitSetFrameBufMulti_vcount);
    if (sceDisplayWaitSetFrameBufMultiCB_used)
        osdDrawStringF(x, y += 22, "sceDisplayWaitSetFrameBufMultiCB( %d )", sceDisplayWaitSetFrameBufMultiCB_vcount);
}

void drawGraphics2Menu(const SceDisplayFrameBuf *pParam) {
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_GRAPHICS_2) / 2, 5, MENU_TITLE_GRAPHICS_2);

    // Header
    osdSetTextScale(1);
    osdDrawStringF(10, 60, "Active Render Targets (%d):", g_renderTargetsCount);
    if (g_renderTargetsCount > MAX_RENDER_TARGETS) {
        char buf[32];
        snprintf(buf, 32, "!! > %d", MAX_RENDER_TARGETS);
        osdDrawStringF(pParam->width - osdGetTextWidth(buf), 60, buf);
    }
    osdDrawStringF(30, 93, "   WxH         MSAA   scenesPF    memBlockUID");

    // Scrollable section
    int x = 30, y = 104;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        osdDrawStringF(pParam->width - 24 - 5, y + 22, "/\\");
        osdDrawStringF(pParam->width - 24 - 5, y + 44, "%2d", g_menuScroll);
    }

    for (int i = g_menuScroll; i < MAX_RENDER_TARGETS; i++) {
        if (g_renderTargets[i].active) {
            osdDrawStringF(x, y += 22, "%4dx%-10d %-8s %-8d 0x%08X %s",
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
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 72, "%2d", MIN(g_renderTargetsCount, MAX_RENDER_TARGETS) - i);
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 50, "\\/");
            break;
        }
    }
}

void drawGraphics3Menu(const SceDisplayFrameBuf *pParam) {
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_GRAPHICS_3) / 2, 5, MENU_TITLE_GRAPHICS_3);

    // Header
    osdSetTextScale(1);
    osdDrawStringF(10, 60, "Color Surfaces (%d):", g_colorSurfacesCount);
    if (g_colorSurfacesCount > MAX_COLOR_SURFACES) {
        char buf[32];
        snprintf(buf, 32, "!! > %d", MAX_COLOR_SURFACES);
        osdDrawStringF(pParam->width - osdGetTextWidth(buf), 60, buf);
    }
    osdDrawStringF(30, 93, "   WxH      stride   MSAA   colorFormat  surfaceType");

    // Scrollable section
    int x = 30, y = 104;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        osdDrawStringF(pParam->width - 24 - 5, y + 22, "/\\");
        osdDrawStringF(pParam->width - 24 - 5, y + 44, "%2d", g_menuScroll);
    }

    for (int i = g_menuScroll; i < MAX_COLOR_SURFACES; i++) {
        if (g_colorSurfaces[i].surface) {
            osdDrawStringF(x, y += 22, "%4dx%-7d %-8d %-6s 0x%-10X 0x%X",
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
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 72, "%2d", MIN(g_colorSurfacesCount, MAX_COLOR_SURFACES) - i);
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 50, "\\/");
            break;
        }
    }
}

void drawGraphics4Menu(const SceDisplayFrameBuf *pParam) {
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_GRAPHICS_4) / 2, 5, MENU_TITLE_GRAPHICS_4);

    // Header
    osdSetTextScale(1);
    osdDrawStringF(10, 60, "Depth/Stencil Surfaces (%d):", g_depthStencilSurfacesCount);
    if (g_depthStencilSurfacesCount > MAX_DEPTH_STENCIL_SURFACES) {
        char buf[32];
        snprintf(buf, 32, "!! > %d", MAX_DEPTH_STENCIL_SURFACES);
        osdDrawStringF(pParam->width - osdGetTextWidth(buf), 60, buf);
    }
    osdDrawStringF(30, 93, "strideSamples  surface     depth       stencil     format      type");

    // Scrollable section
    int x = 30, y = 104;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        osdDrawStringF(pParam->width - 24 - 5, y + 22, "/\\");
        osdDrawStringF(pParam->width - 24 - 5, y + 44, "%2d", g_menuScroll);
    }

    for (int i = g_menuScroll; i < MAX_DEPTH_STENCIL_SURFACES; i++) {
        if (g_depthStencilSurfaces[i].surface) {
            osdDrawStringF(x, y += 22, "    %-9d 0x%-9X 0x%-9X 0x%-9X 0x%-9X 0x%-9X",
                    g_depthStencilSurfaces[i].strideInSamples,
                    g_depthStencilSurfaces[i].surface,
                    g_depthStencilSurfaces[i].depthData,
                    g_depthStencilSurfaces[i].stencilData,
                    g_depthStencilSurfaces[i].depthStencilFormat,
                    g_depthStencilSurfaces[i].surfaceType);
        }

        // Do not draw out of screen
        if (y > pParam->height - 72) {
            // Draw scroll indicator
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 72, "%2d", MIN(g_depthStencilSurfacesCount, MAX_DEPTH_STENCIL_SURFACES) - i);
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 50, "\\/");
            break;
        }
    }
}

void drawGraphics5Menu(const SceDisplayFrameBuf *pParam) {
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_GRAPHICS_5) / 2, 5, MENU_TITLE_GRAPHICS_5);

    // Header
    osdSetTextScale(1);
    osdDrawStringF(10, 60, "Textures (%d):", g_texturesCount);
    if (g_texturesCount > MAX_TEXTURES) {
        char buf[32];
        snprintf(buf, 32, "!! > %d", MAX_TEXTURES);
        osdDrawStringF(pParam->width - osdGetTextWidth(buf), 60, buf);
    }
    osdDrawStringF(30, 93, "  size      type       ptr         data        format  mipCount byteStride");

    // Scrollable section
    int x = 30, y = 104;

    if (g_menuScroll > 0) {
        // Draw scroll indicator
        osdDrawStringF(pParam->width - 24 - 5, y + 22, "/\\");
        osdDrawStringF(pParam->width - 24 - 5, y + 44, "%2d", g_menuScroll);
    }

    for (int i = g_menuScroll; i < MAX_TEXTURES; i++) {
        if (g_textures[i].texture) {
            osdDrawStringF(x, y += 22, "%4dx%-5d %-9s 0x%-9X 0x%-9X 0x%-9X %-6d %d",
                    g_textures[i].width,
                    g_textures[i].height,
                    getTextureTypeString(g_textures[i].type),
                    g_textures[i].texture,
                    g_textures[i].data,
                    g_textures[i].texFormat,
                    g_textures[i].mipCount,
                    g_textures[i].byteStride);
        }

        // Do not draw out of screen
        if (y > pParam->height - 72) {
            // Draw scroll indicator
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 72, "%2d", MIN(g_texturesCount, MAX_TEXTURES) - i);
            osdDrawStringF(pParam->width - 24 - 5, pParam->height - 50, "\\/");
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

    g_hooks[27] = taiHookFunctionImport(
                    &g_hookrefs[27],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xCA9D41D1,
                    sceGxmDepthStencilSurfaceInit_patched);

    g_hooks[28] = taiHookFunctionImport(
                    &g_hookrefs[28],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xD572D547,
                    sceGxmTextureInitSwizzled_patched);
    g_hooks[29] = taiHookFunctionImport(
                    &g_hookrefs[29],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x4811AECB,
                    sceGxmTextureInitLinear_patched);
    g_hooks[30] = taiHookFunctionImport(
                    &g_hookrefs[30],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x6679BEF0,
                    sceGxmTextureInitLinearStrided_patched);
    g_hooks[31] = taiHookFunctionImport(
                    &g_hookrefs[31],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xE6F0DB27,
                    sceGxmTextureInitTiled_patched);
    g_hooks[32] = taiHookFunctionImport(
                    &g_hookrefs[32],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x11DC8DC9,
                    sceGxmTextureInitCube_patched);
}
