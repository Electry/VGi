#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

typedef enum {
    TEXTURE_LINEAR,
    TEXTURE_LINEAR_STRIDED,
    TEXTURE_SWIZZLED,
    TEXTURE_TILED,
    TEXTURE_CUBE
} vgi_texture_type_t;

typedef struct {
    SceGxmTexture *texture;
    uint8_t success;
    vgi_texture_type_t type;
    const void *data;
    SceGxmTextureFormat format;
    unsigned int width;
    unsigned int height;
    unsigned int mip_count;
    unsigned int byte_stride;
} vgi_texture_t;

static vgi_texture_t g_textures[MAX_TEXTURES] = {0};
static int g_textures_count = 0;

static void add_texture(
        int ret,
        vgi_texture_type_t type,
        SceGxmTexture *texture,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount,
        unsigned int byteStride
) {
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
                g_textures_count++;

            g_textures[i].texture = texture;
            g_textures[i].success = ret == SCE_OK;
            g_textures[i].type = type;
            g_textures[i].format = texFormat;
            g_textures[i].width = width;
            g_textures[i].height = height;
            g_textures[i].mip_count = mipCount;
            g_textures[i].byte_stride = byteStride;
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
        unsigned int mipCount
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_SWIZZLED],
                            texture, data, texFormat, width, height, mipCount);

    add_texture(ret, TEXTURE_SWIZZLED, texture, texFormat, width, height, mipCount, 0);
    return ret;
}

static int sceGxmTextureInitLinear_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_LINEAR],
                            texture, data, texFormat, width, height, mipCount);

    add_texture(ret, TEXTURE_LINEAR, texture, texFormat, width, height, mipCount, 0);
    return ret;
}

static int sceGxmTextureInitLinearStrided_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int byteStride
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_LINEAR_STRIDED],
                            texture, data, texFormat, width, height, byteStride);

    add_texture(ret, TEXTURE_LINEAR_STRIDED, texture, texFormat, width, height, 0, byteStride);
    return ret;
}

static int sceGxmTextureInitTiled_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_TILED],
                            texture, data, texFormat, width, height, mipCount);

    add_texture(ret, TEXTURE_TILED, texture, texFormat, width, height, mipCount, 0);
    return ret;
}

static int sceGxmTextureInitCube_patched(
        SceGxmTexture *texture,
        const void *data,
        SceGxmTextureFormat texFormat,
        unsigned int width,
        unsigned int height,
        unsigned int mipCount
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_TEXTURE_INIT_CUBE],
                            texture, data, texFormat, width, height, mipCount);

    add_texture(ret, TEXTURE_CUBE, texture, texFormat, width, height, mipCount, 0);
    return ret;
}

static const char *get_texture_type_text(vgi_texture_type_t type) {
    switch (type) {
        case TEXTURE_LINEAR:         return "LINEAR";
        case TEXTURE_LINEAR_STRIDED: return "STRIDED";
        case TEXTURE_SWIZZLED:       return "SWIZZLED";
        case TEXTURE_TILED:          return "TILED";
        case TEXTURE_CUBE:           return "CUBE";
        default:                     return "?";
    }
}

void vgi_dump_graphics_tex() {
    char msg[128];
    snprintf(msg, sizeof(msg), "Textures (%d/%d):\n",
            g_textures_count, MAX_TEXTURES);
    vgi_cmd_send_msg(msg);

    for (int i = 0; i < MAX_TEXTURES; i++) {
        if (g_textures[i].texture) {
            snprintf(msg, sizeof(msg),
                    "  %4dx%-4d  type=%-8s  texture=0x%08lX  format=0x%-8X  mip=%-4d bytestride=%d%s\n",
                    g_textures[i].width,
                    g_textures[i].height,
                    get_texture_type_text(g_textures[i].type),
                    (uint32_t)g_textures[i].texture,
                    g_textures[i].format,
                    g_textures[i].mip_count,
                    g_textures[i].byte_stride,
                    g_textures[i].success ? "" : " - FAIL!");
            vgi_cmd_send_msg(msg);
        }
    }
}

void vgi_draw_graphics_tex(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Textures (%d, showing only 1 per size):", g_textures_count);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 10, 0), GUI_ANCHOR_TY(yoff, 1.5f),
            "  size      type       ptr         format  mipCount byteStride");

    GUI_SCROLLABLE(MAX_TEXTURES, g_textures_count, 1,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_textures[i].texture) {
            if (g_textures[i].success)
                vgi_gui_set_text_color(255, 255, 255, 255);
            else
                vgi_gui_set_text_color(255, 0, 0, 255);

            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 3 + (i - g_scroll)),
                    "%4dx%-5d %-9s 0x%-9X 0x%-9X %-6d %d",
                    g_textures[i].width,
                    g_textures[i].height,
                    get_texture_type_text(g_textures[i].type),
                    g_textures[i].texture,
                    g_textures[i].format,
                    g_textures[i].mip_count,
                    g_textures[i].byte_stride);
        }
    }
}

void vgi_setup_graphics_tex() {
    HOOK_FUNCTION_IMPORT(0xD572D547, HOOK_SCE_GXM_TEXTURE_INIT_SWIZZLED, sceGxmTextureInitSwizzled_patched);
    HOOK_FUNCTION_IMPORT(0x4811AECB, HOOK_SCE_GXM_TEXTURE_INIT_LINEAR, sceGxmTextureInitLinear_patched);
    HOOK_FUNCTION_IMPORT(0x6679BEF0, HOOK_SCE_GXM_TEXTURE_INIT_LINEAR_STRIDED, sceGxmTextureInitLinearStrided_patched);
    HOOK_FUNCTION_IMPORT(0xE6F0DB27, HOOK_SCE_GXM_TEXTURE_INIT_TILED, sceGxmTextureInitTiled_patched);
    HOOK_FUNCTION_IMPORT(0x11DC8DC9, HOOK_SCE_GXM_TEXTURE_INIT_CUBE, sceGxmTextureInitCube_patched);
}
