#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>

#include "gui.h"
#include "main.h"
#include "log.h"
#include "net/net.h"
#include "net/cmd.h"

// OSD hook
SceUID         g_sceDisplaySetFrameBuf_hook = {0};
tai_hook_ref_t g_sceDisplaySetFrameBuf_hookref = {0};

// Core hooks
SceUID         g_hooks[HOOK_MAX] = {0};
tai_hook_ref_t g_hookrefs[HOOK_MAX] = {0};

uint8_t g_scroll = 0;
uint8_t g_visible = 0;

static vgi_section_t g_section = SECTION_MENU;
static uint32_t    g_buttons_pressed = 0;
static uint32_t    g_buttons_last_scroll_time = 0;

static bool g_net_started = false;
static int g_net_delay_frames = 0;

static const char * const SECTION_TITLE[SECTION_MAX] = {
    [SECTION_MENU]            = "VGi",
    [SECTION_APP_INFO]        = "App Info",
    [SECTION_GRAPHICS]        = "Graphics",
    [SECTION_GRAPHICS_RT]     = "Render Targets",
    [SECTION_GRAPHICS_CS]     = "Color Surfaces",
    [SECTION_GRAPHICS_DSS]    = "Depth/Stencil Surfaces",
    [SECTION_GRAPHICS_TEX]    = "Textures",
    [SECTION_GRAPHICS_SCENES] = "Gxm Scenes",
    [SECTION_GRAPHICS_MISC]   = "Gxm Misc",
    [SECTION_GRAPHICS_CG]     = "Shaders",
    [SECTION_MEMORY]          = "Memory",
    [SECTION_THREADS]         = "Threads",
    [SECTION_INPUT]           = "Input",
    [SECTION_DEVICE]          = "Device"
};

static void check_buttons() {
    SceCtrlData ctrl;
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    unsigned int buttons = ctrl.buttons & ~g_buttons_pressed;
    g_buttons_pressed = ctrl.buttons;
    SceUInt32 time_now = sceKernelGetProcessTimeLow();

    // Toggle VGi
    if (ctrl.buttons & SCE_CTRL_SELECT &&
            buttons & SCE_CTRL_LTRIGGER) {
        g_visible = !g_visible;
        return;
    }

    if (g_visible) {
        // Select section
        if (g_section == SECTION_MENU) {
            if (buttons & SCE_CTRL_CROSS) {
                g_section = MAX(SECTION_MENU + 1, MIN(SECTION_MAX - 1, g_scroll));
                g_scroll = 0;
            }
        }
        // Open section menu
        else if (buttons & SCE_CTRL_SQUARE) {
            g_scroll = g_section;
            g_section = SECTION_MENU;
        }

        // Scroll between sections
        if (buttons & SCE_CTRL_RTRIGGER && g_section < SECTION_MAX - 1) {
            g_section++;
        } else if (buttons & SCE_CTRL_LTRIGGER && g_section > SECTION_MENU + 1) {
            g_section--;
        }

        // Scroll UP
        if (ctrl.buttons & SCE_CTRL_UP &&
                (buttons & SCE_CTRL_UP ||
                time_now - g_buttons_last_scroll_time > BUTTONS_FAST_MOVE_DELAY)) {
            if (g_scroll > 0)
                g_scroll--;

            if (buttons & SCE_CTRL_UP)
                g_buttons_last_scroll_time = time_now;
        }
        // Scroll DOWN
        if (ctrl.buttons & SCE_CTRL_DOWN &&
                (buttons & SCE_CTRL_DOWN ||
                time_now - g_buttons_last_scroll_time > BUTTONS_FAST_MOVE_DELAY)) {
            if (g_scroll < 255)
                g_scroll++;

            if (buttons & SCE_CTRL_DOWN)
                g_buttons_last_scroll_time = time_now;
        }

        // Section-specific controls
        switch (g_section) {
            default: break;
            case SECTION_GRAPHICS:
                vgi_check_buttons_graphics(ctrl.buttons, buttons);
                break;
        }
    }
}

static void draw_hints() {
    vgi_gui_set_back_color(0, 0, 0, 0);
    vgi_gui_set_text_color(255, 255, 255, 255);
    vgi_gui_set_text_scale(1.0f);

    switch (g_section) {
        case SECTION_MENU:
            vgi_gui_print(GUI_ANCHOR_CX(21), GUI_ANCHOR_BY(2, 1),
                    "X Select | D-PAD Move");
            break;
        case SECTION_GRAPHICS_RT:
        case SECTION_GRAPHICS_CS:
        case SECTION_GRAPHICS_DSS:
        case SECTION_GRAPHICS_TEX:
        case SECTION_GRAPHICS_MISC:
        case SECTION_GRAPHICS_CG:
        case SECTION_MEMORY:
        case SECTION_THREADS:
            vgi_gui_print(GUI_ANCHOR_CX(20), GUI_ANCHOR_BY(2, 1),
                    "[] Menu | D-PAD Move");
            break;
        default:
            vgi_gui_print(GUI_ANCHOR_CX(7), GUI_ANCHOR_BY(2, 1),
                    "[] Menu");
            break;
    }

    if (vgi_net_is_running()) {
        SceNetCtlInfo ip;
        sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &ip);
        bool connected = vgi_cmd_is_client_connected();

        vgi_gui_printf(GUI_ANCHOR_LX(5, 0), GUI_ANCHOR_BY(2, 1),
                "%s:%d", ip.ip_address, CMD_PORT);
        vgi_gui_printf(GUI_ANCHOR_RX(5, connected ? 9 : 12), GUI_ANCHOR_BY(2, 1),
                "%s", connected ? "Connected" : "Disconnected");
    }
}

static void draw_menu() {
    char label[128];
    vgi_gui_set_text_scale(1.0f);

    // Clamp scroll position
    g_scroll = MAX(SECTION_MENU + 1, MIN(SECTION_MAX - 1, g_scroll));

    for (vgi_section_t i = SECTION_MENU + 1; i < SECTION_MAX; i++) {
        snprintf(label, sizeof(label), "%s%s%s",
                g_scroll == i ? "> " : "",
                SECTION_TITLE[i],
                g_scroll == i ? " <" : "");

        if (g_scroll == i)
            vgi_gui_set_text_color(0, 255, 255, 255);
        else
            vgi_gui_set_text_color(255, 255, 255, 255);

        vgi_gui_printf(GUI_ANCHOR_CX(strlen(label)),
                        GUI_ANCHOR_CY(SECTION_MAX - 1) + GUI_ANCHOR_TY(0, i - 1),
                        label);
    }
}

static int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
    vgi_gui_set_framebuf(pParam);

    // Check for pressed buttons
    check_buttons();

    // Start net if not running
    if (!g_net_started) {
        g_net_delay_frames++;

        if (g_net_delay_frames >= 30 * 5) {
            vgi_net_start();
            g_net_started = true;
        }
    }

    if (!g_visible) {
        // Draw net info
        vgi_gui_set_back_color(0, 0, 0, 0);
        vgi_gui_set_text_color(0, 255, 0, 255);
        vgi_gui_printf(0, 0, vgi_cmd_is_client_connected() ? ":)" : ":(");
        goto CONT;
    }

    // Background
    vgi_gui_set_back_color(0, 0, 0, 225);
    vgi_gui_clear_screen();

    // Watermark
    vgi_gui_set_back_color(0, 0, 0, 0);
    vgi_gui_set_text_color(10, 20, 50, 255);

    vgi_gui_set_text_scale(5.0f);
    vgi_gui_printf(GUI_ANCHOR_LX(10, 0),
                    GUI_ANCHOR_BY2(65, 1, 5.0f),
                    "VGi " VGi_VERSION);

    vgi_gui_set_text_scale(3.0f);
    vgi_gui_printf(GUI_ANCHOR_LX(30, 0),
                    GUI_ANCHOR_BY2(10, 1, 3.0f),
                    "by Electry");

    // Section title
    vgi_gui_set_text_scale(2.0f);
    vgi_gui_set_text_color(255, 255, 255, 255);
    vgi_gui_printf(GUI_ANCHOR_CX2(strlen(SECTION_TITLE[g_section]), 2.0f),
                    GUI_ANCHOR_TY(5, 0),
                    SECTION_TITLE[g_section]);

    // Hints
    draw_hints();

    // Section content
    vgi_gui_set_text_scale(1.0f);
    int xoff  = 10;
    int x2off = 10;
    int yoff  = 75;
    int y2off = 30;

    switch (g_section) {
        case SECTION_MENU:            draw_menu(); break;
        case SECTION_APP_INFO:        vgi_draw_appinfo(xoff, yoff, x2off, y2off); break;
        case SECTION_GRAPHICS:        vgi_draw_graphics(xoff, yoff, x2off, y2off); break;
        case SECTION_GRAPHICS_RT:     vgi_draw_graphics_rt(xoff, yoff, x2off, y2off); break;
        case SECTION_GRAPHICS_CS:     vgi_draw_graphics_cs(xoff, yoff, x2off, y2off); break;
        case SECTION_GRAPHICS_DSS:    vgi_draw_graphics_dss(xoff, yoff, x2off, y2off); break;
        case SECTION_GRAPHICS_TEX:    vgi_draw_graphics_tex(xoff, yoff, x2off, y2off); break;
        case SECTION_GRAPHICS_SCENES: vgi_draw_graphics_scenes(xoff, yoff, x2off, y2off); break;
        case SECTION_GRAPHICS_MISC:   vgi_draw_graphics_misc(xoff, yoff, x2off, y2off); break;
        case SECTION_GRAPHICS_CG:     vgi_draw_graphics_cg(xoff, yoff, x2off, y2off); break;
        case SECTION_MEMORY:          vgi_draw_memory(xoff, yoff, x2off, y2off); break;
        case SECTION_THREADS:         vgi_draw_threads(xoff, yoff, x2off, y2off); break;
        case SECTION_INPUT:           vgi_draw_input(xoff, yoff, x2off, y2off); break;
        case SECTION_DEVICE:          vgi_draw_device(xoff, yoff, x2off, y2off); break;
        default:                      break;
    }

    sceDisplayWaitSetFrameBufMulti(4);
CONT:
    return TAI_CONTINUE(int, g_sceDisplaySetFrameBuf_hookref, pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    // Setup display hook
    g_sceDisplaySetFrameBuf_hook = taiHookFunctionImport(
            &g_sceDisplaySetFrameBuf_hookref,
            TAI_MAIN_MODULE,
            TAI_ANY_LIBRARY,
            0x7A410B64,
            sceDisplaySetFrameBuf_patched);

    // Setup core hooks
    vgi_setup_appinfo();
    vgi_setup_graphics();
    vgi_setup_graphics_rt();
    vgi_setup_graphics_cs();
    vgi_setup_graphics_dss();
    vgi_setup_graphics_tex();
    vgi_setup_graphics_scenes();
    vgi_setup_graphics_misc();
    vgi_setup_graphics_cg();
    vgi_setup_memory();
    vgi_setup_threads();
    vgi_setup_input();
    vgi_setup_device();

    if (strncmp(g_app_titleid, "VITASHELL", 9)) {
        vgi_log_prepare();
        vgi_log_printf("VGi " VGi_VERSION "\n");
        vgi_log_printf("Title: %s\n", g_app_titleid);
        vgi_log_printf("------------------------------------- module_start():end\n");
        vgi_log_flush();
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    // Stop net callback thread
    vgi_net_stop();

    // Release display hook
    if (g_sceDisplaySetFrameBuf_hook >= 0)
        taiHookRelease(g_sceDisplaySetFrameBuf_hook, g_sceDisplaySetFrameBuf_hookref);

    // Release core hooks
    for (int i = 0; i < HOOK_MAX; i++) {
        if (g_hooks[i] >= 0)
            taiHookRelease(g_hooks[i], g_hookrefs[i]);
    }

    // Flush log
    vgi_log_flush();

    return SCE_KERNEL_STOP_SUCCESS;
}
