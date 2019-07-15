#include <vitasdk.h>
#include <taihen.h>

#include "osd.h"
#include "main.h"

// OSD hook
SceUID         sceDisplaySetFrameBuf_hook = {0};
tai_hook_ref_t sceDisplaySetFrameBuf_hookref = {0};

// Core hooks
SceUID         g_hooks[HOOK_MAX] = {0};
tai_hook_ref_t g_hookrefs[HOOK_MAX] = {0};

uint8_t g_scroll = 0;
uint8_t g_visible = 0;

static VGi_Section  g_section = SECTION_MENU;
static unsigned int g_buttonsPressed = 0;
static SceUInt32    g_buttonsLastScrollTime = 0;

static const char * const g_sectionTitle[SECTION_MAX] = {
    "VGi",
    "App Info",
    "Graphics",
    "Render Targets",
    "Color Surfaces",
    "Depth/Stencil Surfaces",
    "Textures",
    "Gxm Scenes",
    "Gxm Misc",
    "Shaders",
    "Memory",
    "Threads",
    "Input",
    "Device"
};

void hookFunctionImport(uint32_t nid, uint32_t id, const void *func) {
    if (g_hooks[id] > 0)
        return;

    g_hooks[id] = taiHookFunctionImport(&g_hookrefs[id], TAI_MAIN_MODULE,
                                        TAI_ANY_LIBRARY, nid, func);
}

static void checkButtons() {
    SceCtrlData ctrl;
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    unsigned int buttons = ctrl.buttons & ~g_buttonsPressed;
    g_buttonsPressed = ctrl.buttons;
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

        // Scroll UP
        if (ctrl.buttons & SCE_CTRL_UP &&
                (buttons & SCE_CTRL_UP ||
                time_now - g_buttonsLastScrollTime > BUTTONS_FAST_MOVE_DELAY)) {
            if (g_scroll > 0)
                g_scroll--;

            if (buttons & SCE_CTRL_UP)
                g_buttonsLastScrollTime = time_now;
        }
        // Scroll DOWN
        if (ctrl.buttons & SCE_CTRL_DOWN &&
                (buttons & SCE_CTRL_DOWN ||
                time_now - g_buttonsLastScrollTime > BUTTONS_FAST_MOVE_DELAY)) {
            if (g_scroll < 255)
                g_scroll++;

            if (buttons & SCE_CTRL_DOWN)
                g_buttonsLastScrollTime = time_now;
        }

        // Section-specific controls
        if (g_section == SECTION_GRAPHICS) {
            checkButtonsGraphics(ctrl.buttons, buttons);
        }
    }
}

void drawScrollIndicator(int x, int y, uint8_t scroll_down, int count) {
    if (count <= 0)
        return;

    if (scroll_down) {
        osdDrawStringF(x, y, "%2d", count);
        osdDrawStringF(x, y + osdGetLineHeight(), "\\/");
    } else {
        osdDrawStringF(x, y, "/\\");
        osdDrawStringF(x, y + osdGetLineHeight(), "%2d", count);
    }
}

static void drawHints(const SceDisplayFrameBuf *pParam) {
    char buf[64];
    osdSetTextColor(255, 255, 255, 255);
    osdSetTextScale(1);

    if (g_section == SECTION_MENU) {
        snprintf(buf, 64, "X Select");
    } else {
        snprintf(buf, 64, "[] Menu");
    }

    // Scrollable sections
    if (g_section == SECTION_MENU ||
            g_section == SECTION_GRAPHICS_RT ||
            g_section == SECTION_GRAPHICS_CS ||
            g_section == SECTION_GRAPHICS_DSS ||
            g_section == SECTION_GRAPHICS_TEX ||
            g_section == SECTION_GRAPHICS_MISC ||
            g_section == SECTION_GRAPHICS_CG ||
            g_section == SECTION_MEMORY ||
            g_section == SECTION_THREADS) {
        snprintf(buf, 64, "%s | D-PAD Move", buf);
    }

    // Draw text
    osdSetBgColor(0, 0, 0, 0);
    osdDrawStringF((pParam->width / 2) - (osdGetTextWidth(buf) / 2),
                    pParam->height - osdGetLineHeight() - 2, "%s", buf);
}

static void drawMenu(const SceDisplayFrameBuf *pParam) {

    osdSetTextColor(255, 255, 255, 255);
    osdSetTextScale(1);
    int x = (pParam->width / 2);
    int y = (pParam->height / 2) - (((osdGetLineHeight() + 4) * SECTION_MAX) / 2);
    char title[128];

    // Update global
    g_scroll = MAX(SECTION_MENU + 1, MIN(SECTION_MAX - 1, g_scroll));

    for (VGi_Section i = SECTION_MENU + 1; i < SECTION_MAX; i++) {
        snprintf(title, 128, "%s%s%s",
                g_scroll == i ? "> " : "",
                g_sectionTitle[i],
                g_scroll == i ? " <" : "");

        if (g_scroll == i)
            osdSetTextColor(0, 255, 255, 255);
        else
            osdSetTextColor(255, 255, 255, 255);

        osdDrawStringF(x - (osdGetTextWidth(title) / 2),
                    y += osdGetLineHeight() + 4,
                    title);
    }
}

static int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {

    // Check for pressed buttons
    checkButtons();

    if (!g_visible)
        goto CONT;

    osdUpdateFrameBuf(pParam);

    // BG
    osdSetBgColor(0, 0, 0, 225);
    osdFastDrawRectangle(0, 0, pParam->width, pParam->height);

    // Watermark
    osdSetBgColor(0, 0, 0, 0);
    osdSetTextColor(10, 20, 50, 255);
    osdSetTextScale(3);
    int byH = osdGetLineHeight();
    osdDrawStringF(30, pParam->height - byH - 10, "by Electry");
    osdSetTextScale(5);
    osdDrawStringF(10, pParam->height - byH - osdGetLineHeight(),
                    "VGi %s", VGi_VERSION);

    // Section region
    int minX = 10, minY, maxX = pParam->width - 10, maxY;

    // Title
    osdSetTextScale(2);
    osdSetTextColor(255, 255, 255, 255);
    osdDrawStringF((pParam->width / 2) - (osdGetTextWidth(g_sectionTitle[g_section]) / 2), 5,
                    g_sectionTitle[g_section]);
    minY = osdGetLineHeight() + 10;

    // Hints
    drawHints(pParam);
    maxY = pParam->height - osdGetLineHeight() * 2;

    // Section content
    osdSetTextScale(1);
    osdSetTextColor(255, 255, 255, 255);

    switch (g_section) {
        case SECTION_MENU:            drawMenu(pParam); break;
        case SECTION_APP_INFO:        drawAppInfo(minX, minY, maxX, maxY); break;
        case SECTION_GRAPHICS:        drawGraphics(minX, minY, maxX, maxY, pParam); break;
        case SECTION_GRAPHICS_RT:     drawGraphicsRT(minX, minY, maxX, maxY); break;
        case SECTION_GRAPHICS_CS:     drawGraphicsCS(minX, minY, maxX, maxY); break;
        case SECTION_GRAPHICS_DSS:    drawGraphicsDSS(minX, minY, maxX, maxY); break;
        case SECTION_GRAPHICS_TEX:    drawGraphicsTex(minX, minY, maxX, maxY); break;
        case SECTION_GRAPHICS_SCENES: drawGraphicsScenes(minX, minY, maxX, maxY); break;
        case SECTION_GRAPHICS_MISC:   drawGraphicsMisc(minX, minY, maxX, maxY); break;
        case SECTION_GRAPHICS_CG:     drawGraphicsCg(minX, minY, maxX, maxY); break;
        case SECTION_MEMORY:          drawMemory(minX, minY, maxX, maxY); break;
        case SECTION_THREADS:         drawThreads(minX, minY, maxX, maxY); break;
        case SECTION_INPUT:           drawInput(minX, minY, maxX, maxY); break;
        case SECTION_DEVICE:          drawDevice(minX, minY, maxX, maxY); break;
        case SECTION_MAX:             break;
    }

    sceDisplayWaitSetFrameBufMulti(4);
CONT:
    return TAI_CONTINUE(int, sceDisplaySetFrameBuf_hookref, pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {

    sceDisplaySetFrameBuf_hook = taiHookFunctionImport(
                    &sceDisplaySetFrameBuf_hookref,
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x7A410B64,
                    sceDisplaySetFrameBuf_patched);

    // Setup hooks, etc...
    setupAppInfo();
    setupGraphics();
    setupGraphicsRT();
    setupGraphicsCS();
    setupGraphicsDSS();
    setupGraphicsTex();
    setupGraphicsScenes();
    setupGraphicsMisc();
    setupGraphicsCg();
    setupMemory();
    setupThreads();
    setupInput();
    setupDevice();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    // Release OSD hook
    if (sceDisplaySetFrameBuf_hook >= 0)
        taiHookRelease(sceDisplaySetFrameBuf_hook, sceDisplaySetFrameBuf_hookref);

    // Release core hooks
    for (int i = 0; i < HOOK_MAX; i++) {
        if (g_hooks[i] >= 0)
            taiHookRelease(g_hooks[i], g_hookrefs[i]);
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
