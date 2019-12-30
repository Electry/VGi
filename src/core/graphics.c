#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

static uint8_t g_sceDisplayGetRefreshRate_used = 0;
static uint8_t g_sceDisplayGetVcount_used = 0;
static uint8_t g_sceDisplayWaitVblankStart_used = 0;
static uint8_t g_sceDisplayWaitVblankStartCB_used = 0;
static uint8_t g_sceDisplayWaitVblankStartMulti_used = 0;
static uint8_t g_sceDisplayWaitVblankStartMultiCB_used = 0;
static uint8_t g_sceDisplayWaitSetFrameBuf_used = 0;
static uint8_t g_sceDisplayWaitSetFrameBufCB_used = 0;
static uint8_t g_sceDisplayWaitSetFrameBufMulti_used = 0;
static uint8_t g_sceDisplayWaitSetFrameBufMultiCB_used = 0;

static uint8_t g_sceDisplayWaitVblankStartMulti_vcount = 0;
static uint8_t g_sceDisplayWaitVblankStartMultiCB_vcount = 0;
static uint8_t g_sceDisplayWaitSetFrameBufMulti_vcount = 0;
static uint8_t g_sceDisplayWaitSetFrameBufMultiCB_vcount = 0;

static SceGxmInitializeParams g_gxm_initialize_params = {0};

#define VSYNC_DROP 3
static uint8_t g_vsync_killswitch = 0; // 0 = off, x = force x, VSYNC_DROP = drop all

static int sceDisplayGetRefreshRate_patched(float *pFps) {
    g_sceDisplayGetRefreshRate_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_GET_REFRESH_RATE], pFps);
}

static int sceDisplayGetVcount_patched() {
    g_sceDisplayGetVcount_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_GET_VCOUNT]);
}

static int sceDisplayWaitVblankStart_patched() {
    if (g_vsync_killswitch == VSYNC_DROP)
        return 0;

    g_sceDisplayWaitVblankStart_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_VBLANK_START]);
}

static int sceDisplayWaitVblankStartCB_patched() {
    if (g_vsync_killswitch == VSYNC_DROP)
        return 0;

    g_sceDisplayWaitVblankStartCB_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_VBLANK_START_CB]);
}

static int sceDisplayWaitVblankStartMulti_patched(unsigned int vcount) {
    if (g_vsync_killswitch == VSYNC_DROP)
        return 0;

    if (g_vsync_killswitch)
        vcount = g_vsync_killswitch;

    g_sceDisplayWaitVblankStartMulti_used = 1;
    g_sceDisplayWaitVblankStartMulti_vcount = vcount;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI], vcount);
}

static int sceDisplayWaitVblankStartMultiCB_patched(unsigned int vcount) {
    if (g_vsync_killswitch == VSYNC_DROP)
        return 0;

    if (g_vsync_killswitch)
        vcount = g_vsync_killswitch;

    g_sceDisplayWaitVblankStartMultiCB_used = 1;
    g_sceDisplayWaitVblankStartMultiCB_vcount = vcount;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI_CB], vcount);
}

static int sceDisplayWaitSetFrameBuf_patched() {
    if (g_vsync_killswitch == VSYNC_DROP)
        return 0;

    g_sceDisplayWaitSetFrameBuf_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF]);
}

static int sceDisplayWaitSetFrameBufCB_patched() {
    if (g_vsync_killswitch == VSYNC_DROP)
        return 0;

    g_sceDisplayWaitSetFrameBufCB_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_CB]);
}

static int sceDisplayWaitSetFrameBufMulti_patched(unsigned int vcount) {
    if (g_vsync_killswitch == VSYNC_DROP)
        return 0;

    if (g_vsync_killswitch)
        vcount = g_vsync_killswitch;

    g_sceDisplayWaitSetFrameBufMulti_used = 1;
    g_sceDisplayWaitSetFrameBufMulti_vcount = vcount;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI], vcount);
}

static int sceDisplayWaitSetFrameBufMultiCB_patched(unsigned int vcount) {
    if (g_vsync_killswitch == VSYNC_DROP)
        return 0;

    if (g_vsync_killswitch)
        vcount = g_vsync_killswitch;

    g_sceDisplayWaitSetFrameBufMultiCB_used = 1;
    g_sceDisplayWaitSetFrameBufMultiCB_vcount = vcount;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI_CB], vcount);
}

static int sceGxmInitialize_patched(const SceGxmInitializeParams *params) {
    memcpy(&g_gxm_initialize_params, params, sizeof(SceGxmInitializeParams));
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_INITIALIZE], params);
}

static void draw_graphics_vsync_item(
        int *x, int *y,
        const char *str,
        uint8_t *item, uint8_t *vcount
) {
    if (!*item)
        return; // Never called

    if (*item == 1) // Called since last setFrameBuf
        vgi_gui_set_text_color(255, 255, 255, 255);
    else // Called some time before
        vgi_gui_set_text_color(155, 155, 155, 255);

    if (vcount != NULL) {
        vgi_gui_printf(*x, *y, str, *vcount);
    } else {
        vgi_gui_printf(*x, *y, str);
    }
    *y += GUI_FONT_H;

    // Reset usage
    *item = 2;
}

void vgi_check_buttons_graphics(uint32_t buttons_now, uint32_t buttons) {
    // Vsync Killswitch
    if (buttons & SCE_CTRL_CIRCLE) {
        // No "multi" func is used, skip multi opts
        if (!g_sceDisplayWaitVblankStartMulti_used &&
                !g_sceDisplayWaitVblankStartMultiCB_used &&
                !g_sceDisplayWaitSetFrameBufMulti_used &&
                !g_sceDisplayWaitSetFrameBufMultiCB_used) {
            g_vsync_killswitch = !g_vsync_killswitch ? VSYNC_DROP : 0;
        }
        // otherwise
        else {
            if (g_vsync_killswitch < VSYNC_DROP)
                g_vsync_killswitch++;
            else
                g_vsync_killswitch = 0;
        }
    }
}

void vgi_dump_graphics() {
    char msg[2048];
    snprintf(msg, sizeof(msg),
            "Graphics:\n"
            "  Framebuffer:        %dx%d, stride = %d\n"
            "  Max Pending Swaps:  %d\n"
            "  DQ Callback:        0x%lX\n"
            "  Param. buf. size:   %d B = 0x%X\n"
            "Vsync:\n",
            g_gui_framebuf.width, g_gui_framebuf.height, g_gui_framebuf.pitch,
            g_gxm_initialize_params.displayQueueMaxPendingCount,
            (uint32_t)g_gxm_initialize_params.displayQueueCallback,
            g_gxm_initialize_params.parameterBufferSize, g_gxm_initialize_params.parameterBufferSize);
    vgi_cmd_send_msg(msg);

    if (g_sceDisplayGetRefreshRate_used)    vgi_cmd_send_msg("  sceDisplayGetRefreshRate()\n");
    if (g_sceDisplayGetVcount_used)         vgi_cmd_send_msg("  sceDisplayGetVcount()\n");
    if (g_sceDisplayWaitVblankStart_used)   vgi_cmd_send_msg("  sceDisplayWaitVblankStart()\n");
    if (g_sceDisplayWaitVblankStartCB_used) vgi_cmd_send_msg("  sceDisplayWaitVblankStartCB()\n");
    if (g_sceDisplayWaitVblankStartMulti_used) {
        snprintf(msg, sizeof(msg), "  sceDisplayWaitVblankStartMulti( %d )\n", g_sceDisplayWaitVblankStartMulti_vcount);
        vgi_cmd_send_msg(msg);
    }
    if (g_sceDisplayWaitVblankStartMultiCB_used) {
        snprintf(msg, sizeof(msg), "  sceDisplayWaitVblankStartMultiCB( %d )\n", g_sceDisplayWaitVblankStartMultiCB_vcount);
        vgi_cmd_send_msg(msg);
    }
    if (g_sceDisplayWaitSetFrameBuf_used)   vgi_cmd_send_msg("  sceDisplayWaitSetFrameBuf()\n");
    if (g_sceDisplayWaitSetFrameBufCB_used) vgi_cmd_send_msg("  sceDisplayWaitSetFrameBufCB()\n");
    if (g_sceDisplayWaitSetFrameBufMulti_used) {
        snprintf(msg, sizeof(msg), "  sceDisplayWaitSetFrameBufMulti( %d )\n", g_sceDisplayWaitSetFrameBufMulti_vcount);
        vgi_cmd_send_msg(msg);
    }
    if (g_sceDisplayWaitSetFrameBufMultiCB_used) {
        snprintf(msg, sizeof(msg), "  sceDisplayWaitSetFrameBufMultiCB( %d )\n", g_sceDisplayWaitSetFrameBufMultiCB_vcount);
        vgi_cmd_send_msg(msg);
    }
}

void vgi_draw_graphics(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Framebuffer:        %dx%d, stride = %d", g_gui_framebuf.width, g_gui_framebuf.height, g_gui_framebuf.pitch);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 1),
            "Max Pending Swaps:  %d", g_gxm_initialize_params.displayQueueMaxPendingCount);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 2),
            "DQ Callback:        0x%X", g_gxm_initialize_params.displayQueueCallback);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3),
            "Param. buf. size:   %d B = 0x%X", g_gxm_initialize_params.parameterBufferSize,
                                               g_gxm_initialize_params.parameterBufferSize);

    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 4.5f), "VSync:");

    if (!g_vsync_killswitch) {
        vgi_gui_set_text_color(150, 255, 150, 255);
        vgi_gui_printf(GUI_ANCHOR_RX(x2off, 12), GUI_ANCHOR_TY(yoff, 4.5f), "DEFAULT (O)");
    } else if (g_vsync_killswitch == 3) {
        vgi_gui_set_text_color(255, 150, 150, 255);
        vgi_gui_printf(GUI_ANCHOR_RX(x2off, 13), GUI_ANCHOR_TY(yoff, 4.5f), "DROP ALL (O)");
    } else {
        vgi_gui_set_text_color(255, 255, 150, 255);
        vgi_gui_printf(GUI_ANCHOR_RX(x2off, 12), GUI_ANCHOR_TY(yoff, 4.5f), "FORCE %d (O)", g_vsync_killswitch);
    }
    vgi_gui_set_text_color(255, 255, 255, 255);

    int x = GUI_ANCHOR_LX(xoff + 20, 0);
    int y = GUI_ANCHOR_TY(yoff, 6);
    draw_graphics_vsync_item(&x, &y, "sceDisplayGetRefreshRate()",
                            &g_sceDisplayGetRefreshRate_used, NULL);
    draw_graphics_vsync_item(&x, &y, "sceDisplayGetVcount()",
                            &g_sceDisplayGetVcount_used, NULL);
    draw_graphics_vsync_item(&x, &y, "sceDisplayWaitVblankStart()",
                            &g_sceDisplayWaitVblankStart_used, NULL);
    draw_graphics_vsync_item(&x, &y, "sceDisplayWaitVblankStartCB()",
                            &g_sceDisplayWaitVblankStartCB_used, NULL);
    draw_graphics_vsync_item(&x, &y, "sceDisplayWaitVblankStartMulti( %d )",
                            &g_sceDisplayWaitVblankStartMulti_used, &g_sceDisplayWaitVblankStartMulti_vcount);
    draw_graphics_vsync_item(&x, &y, "sceDisplayWaitVblankStartMultiCB( %d )",
                            &g_sceDisplayWaitVblankStartMultiCB_used, &g_sceDisplayWaitVblankStartMultiCB_vcount);
    draw_graphics_vsync_item(&x, &y, "sceDisplayWaitSetFrameBuf()",
                            &g_sceDisplayWaitSetFrameBuf_used, NULL);
    draw_graphics_vsync_item(&x, &y, "sceDisplayWaitSetFrameBufCB()",
                            &g_sceDisplayWaitSetFrameBufCB_used, NULL);
    draw_graphics_vsync_item(&x, &y, "sceDisplayWaitSetFrameBufMulti( %d )",
                            &g_sceDisplayWaitSetFrameBufMulti_used, &g_sceDisplayWaitSetFrameBufMulti_vcount);
    draw_graphics_vsync_item(&x, &y, "sceDisplayWaitSetFrameBufMultiCB( %d )",
                            &g_sceDisplayWaitSetFrameBufMultiCB_used, &g_sceDisplayWaitSetFrameBufMultiCB_vcount);
}

void vgi_setup_graphics() {
    HOOK_FUNCTION_IMPORT(0xA08CA60D, HOOK_SCE_DISPLAY_GET_REFRESH_RATE, sceDisplayGetRefreshRate_patched);
    HOOK_FUNCTION_IMPORT(0xB6FDE0BA, HOOK_SCE_DISPLAY_GET_VCOUNT, sceDisplayGetVcount_patched);
    HOOK_FUNCTION_IMPORT(0x5795E898, HOOK_SCE_DISPLAY_WAIT_VBLANK_START, sceDisplayWaitVblankStart_patched);
    HOOK_FUNCTION_IMPORT(0x78B41B92, HOOK_SCE_DISPLAY_WAIT_VBLANK_START_CB, sceDisplayWaitVblankStartCB_patched);
    HOOK_FUNCTION_IMPORT(0xDD0A13B8, HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI, sceDisplayWaitVblankStartMulti_patched);
    HOOK_FUNCTION_IMPORT(0x05F27764, HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI_CB, sceDisplayWaitVblankStartMultiCB_patched);
    HOOK_FUNCTION_IMPORT(0x9423560C, HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF, sceDisplayWaitSetFrameBuf_patched);
    HOOK_FUNCTION_IMPORT(0x814C90AF, HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_CB, sceDisplayWaitSetFrameBufCB_patched);
    HOOK_FUNCTION_IMPORT(0x7D9864A8, HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI, sceDisplayWaitSetFrameBufMulti_patched);
    HOOK_FUNCTION_IMPORT(0x3E796EF5, HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI_CB, sceDisplayWaitSetFrameBufMultiCB_patched);
    HOOK_FUNCTION_IMPORT(0xB0F1E4EC, HOOK_SCE_GXM_INITIALIZE, sceGxmInitialize_patched);
}
