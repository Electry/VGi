#include <psp2/kernel/modulemgr.h>
#include <psp2/ctrl.h>
#include <libk/stdio.h>
#include <vitasdk.h>
#include <taihen.h>

#include "display.h"
#include "main.h"

#define VGi_VERSION  "v0.3"

#define HOOKS_NUM 26
SceUID g_hooks[HOOKS_NUM] = {0};
tai_hook_ref_t g_hookrefs[HOOKS_NUM] = {0};

uint8_t g_menuScroll = 0;
static uint8_t g_menuVisible = 0;
static VGi_MenuSection g_menuSection = MENU_APP_INFO;
static unsigned int g_buttonsPressed = 0;

static void checkButtons() {
    SceCtrlData ctrl;
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    unsigned int buttons = ctrl.buttons & ~g_buttonsPressed;

    if (ctrl.buttons & SCE_CTRL_SELECT) {
        // Toggle menu
        if (buttons & SCE_CTRL_LTRIGGER) {
            g_menuVisible = !g_menuVisible;
        }
    } else if (g_menuVisible) {
        // Move between sections
        if (buttons & SCE_CTRL_LTRIGGER) {
            if (g_menuSection > MENU_APP_INFO) {
                g_menuSection--;
                g_menuScroll = 0;
            }
        } else if (buttons & SCE_CTRL_RTRIGGER) {
            if (g_menuSection < MENU_MAX - 1) {
                g_menuSection++;
                g_menuScroll = 0;
            }
        } else if (buttons & SCE_CTRL_UP) {
            if (g_menuScroll > 0)
                g_menuScroll--;
        } else if (buttons & SCE_CTRL_DOWN) {
            if (g_menuScroll < 255)
                g_menuScroll++;
        }
    }

    g_buttonsPressed = ctrl.buttons;
}

static void drawNextSectionIndicator(const SceDisplayFrameBuf *pParam, const char *L, const char *R) {
    char buf[64];

    snprintf(buf, 64, "%s%s", strlen(L) > 0 ? "< " : "", L);
    drawStringF(0, pParam->height - 22, "%s", buf);

    snprintf(buf, 64, "%s%s", R, strlen(R) > 0 ? " >" : "");
    drawStringF(pParam->width - getTextWidth(buf), pParam->height - 22, "%s", buf);
}

static int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {

    // Check for pressed buttons
    checkButtons();

    if (!g_menuVisible)
        goto CONT;

    updateFrameBuf(pParam);

    // BG
    setBgColor(0, 0, 0, 255);
    clearScreen();

    // Watermark
    setBgColor(0, 0, 0, 0);
    setTextColor(10, 20, 50, 255);
    setTextScale(5);
    drawStringF(0, pParam->height - 110, "VGi %s", VGi_VERSION);
    setTextScale(2);
    drawStringF(pParam->width - getTextWidth("by Electry"), pParam->height - 66, "by Electry");

    // Content
    setTextColor(255, 255, 255, 255);

    if (g_menuSection == MENU_APP_INFO) {
        drawAppInfoMenu(pParam);
        drawNextSectionIndicator(pParam, "", MENU_TITLE_GRAPHICS);

    } else if (g_menuSection == MENU_GRAPHICS) {
        drawGraphicsMenu(pParam);
        drawNextSectionIndicator(pParam, MENU_TITLE_APP_INFO, MENU_TITLE_GRAPHICS_2);

    } else if (g_menuSection == MENU_GRAPHICS_2) {
        drawGraphics2Menu(pParam);
        drawNextSectionIndicator(pParam, MENU_TITLE_GRAPHICS, MENU_TITLE_GRAPHICS_3);

    } else if (g_menuSection == MENU_GRAPHICS_3) {
        drawGraphics3Menu(pParam);
        drawNextSectionIndicator(pParam, MENU_TITLE_GRAPHICS_2, MENU_TITLE_MEMORY);

    } else if (g_menuSection == MENU_MEMORY) {
        drawMemoryMenu(pParam);
        drawNextSectionIndicator(pParam, MENU_TITLE_GRAPHICS_3, MENU_TITLE_MISC);

    } else if (g_menuSection == MENU_MISC) {
        drawMiscMenu(pParam);
        drawNextSectionIndicator(pParam, MENU_TITLE_MEMORY, MENU_TITLE_DEVICE);

    } else if (g_menuSection == MENU_DEVICE) {
        setTextScale(2);
        drawStringF((pParam->width / 2) - getTextWidth(MENU_TITLE_DEVICE) / 2, 5, MENU_TITLE_DEVICE);

        setTextScale(1);
        drawNextSectionIndicator(pParam, MENU_TITLE_MISC, "");
    }

CONT:
    return TAI_CONTINUE(int, g_hookrefs[0], pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {

    g_hooks[0] = taiHookFunctionImport(
                    &g_hookrefs[0],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x7A410B64,
                    sceDisplaySetFrameBuf_patched);

    // Setup hooks, etc...
    setupAppInfoMenu();
    setupGraphicsMenu();
    setupMemoryMenu();
    setupMiscMenu();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    for (int i = 0; i < HOOKS_NUM; i++) {
        if (g_hooks[i] >= 0)
            taiHookRelease(g_hooks[i], g_hookrefs[i]);
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
