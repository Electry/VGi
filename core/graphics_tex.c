#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef enum {
    TEXTURE_LINEAR,
    TEXTURE_LINEAR_STRIDED,
    TEXTURE_SWIZZLED,
    TEXTURE_TILED,
    TEXTURE_CUBE
} VGi_TextureType;

typedef struct {
    SceGxmTexture *texture;
    uint8_t success;
    VGi_TextureType type;
    const void *data;
    SceGxmTextureFormat texFormat;
    unsigned int width;
    unsigned int height;
    unsigned int mipCount;
    unsigned int byteStride;
} VGi_Texture;

static VGi_Texture g_textures[MAX_TEXTURES] = {0};
static uint32_t g_texturesCount = 0;

static void addTexture(
        int ret,
        VGi_TextureType type,
        SceGxmTexture *texture,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount,
        unsigned int byteStride) {

    for (int i = 0; i < MAX_TEXTURES; i++) {
        if (g_textures[i].texture &&
                g_textures[i].texture != texture &&
                g_textures[i].width == width &&
                g_textures[i].height == height) {
            break; // Skip already known size
        }

        if (g_textures[i].texture == texture || // Existing (reinit)
                !g_textures[i].texture) { // New
            if (!g_textures[i].texture)
                g_texturesCount++;

            g_textures[i].texture = texture;
            g_textures[i].success = ret == SCE_OK;
            g_textures[i].type = type;
            g_textures[i].texFormat = texFormat;
            g_textures[i].width = width;
            g_textures[i].height = height;
            g_textures[i].mipCount = mipCount;
            g_textures[i].byteStride = byteStride;
            break;
        }
    }
}

static int sceGxmTextureInitSwizzled_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_SWIZZLED],
                            texture, data, texFormat, width, height, mipCount);
    addTexture(ret, TEXTURE_SWIZZLED, texture, texFormat, width, height, mipCount, 0);
    return ret;
}
static int sceGxmTextureInitLinear_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_LINEAR],
                            texture, data, texFormat, width, height, mipCount);
    addTexture(ret, TEXTURE_LINEAR, texture, texFormat, width, height, mipCount, 0);
    return ret;
}
static int sceGxmTextureInitLinearStrided_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int byteStride) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_LINEAR_STRIDED],
                            texture, data, texFormat, width, height, byteStride);
    addTexture(ret, TEXTURE_LINEAR_STRIDED, texture, texFormat, width, height, 0, byteStride);
    return ret;
}
static int sceGxmTextureInitTiled_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_TILED],
                            texture, data, texFormat, width, height, mipCount);
    addTexture(ret, TEXTURE_TILED, texture, texFormat, width, height, mipCount, 0);
    return ret;
}
static int sceGxmTextureInitCube_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_CUBE],
                            texture, data, texFormat, width, height, mipCount);
    addTexture(ret, TEXTURE_CUBE, texture, texFormat, width, height, mipCount, 0);
    return ret;
}

static const char *getTextureTypeString(VGi_TextureType type) {
    switch (type) {
        case TEXTURE_LINEAR:
            return "LINEAR";
        case TEXTURE_LINEAR_STRIDED:
            return "STRIDED";
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

void drawGraphicsTex(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Textures (%d, showing only 1 per size):", g_texturesCount);
    osdDrawStringF(x += 10, y += osdGetLineHeight() * 1.5f,
                    "  size      type       ptr         format  mipCount byteStride");

    // Scrollable section
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    for (int i = g_scroll; i < MAX_TEXTURES; i++) {
        if (y > maxY - osdGetLineHeight()) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_texturesCount - i);
            break;
        }

        if (g_textures[i].texture) {
            if (g_textures[i].success)
                osdSetTextColor(255, 255, 255, 255);
            else
                osdSetTextColor(255, 0, 0, 255);

            osdDrawStringF(x, y += osdGetLineHeight(), "%4dx%-5d %-9s 0x%-9X 0x%-9X %-6d %d",
                    g_textures[i].width,
                    g_textures[i].height,
                    getTextureTypeString(g_textures[i].type),
                    g_textures[i].texture,
                    g_textures[i].texFormat,
                    g_textures[i].mipCount,
                    g_textures[i].byteStride);
        }
    }
}

void setupGraphicsTex() {
    hookFunctionImport(0xD572D547, HOOK_SCE_GXM_TEXTURE_INIT_SWIZZLED, sceGxmTextureInitSwizzled_patched);
    hookFunctionImport(0x4811AECB, HOOK_SCE_GXM_TEXTURE_INIT_LINEAR, sceGxmTextureInitLinear_patched);
    hookFunctionImport(0x6679BEF0, HOOK_SCE_GXM_TEXTURE_INIT_LINEAR_STRIDED, sceGxmTextureInitLinearStrided_patched);
    hookFunctionImport(0xE6F0DB27, HOOK_SCE_GXM_TEXTURE_INIT_TILED, sceGxmTextureInitTiled_patched);
    hookFunctionImport(0x11DC8DC9, HOOK_SCE_GXM_TEXTURE_INIT_CUBE, sceGxmTextureInitCube_patched);
}
