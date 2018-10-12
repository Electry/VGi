#include <libk/stdio.h>
#include <vitasdk.h>
#include <taihen.h>

#include "display.h"
#include "main.h"

// app info
static tai_module_info_t g_appModuleInfo = {0};
static SceKernelModuleInfo g_appSceModuleInfo = {0};
static char g_appContent[256] = {0};
static char g_appTitle[256] = {0};
static char g_appSTitle[64] = {0};
static char g_appTitleID[12] = {0};
static char g_appRegion[10] = {0};

void drawAppInfoMenu(const SceDisplayFrameBuf *pParam) {
    setTextScale(2);
    drawStringF((pParam->width / 2) - getTextWidth(MENU_TITLE_APP_INFO) / 2, 5, MENU_TITLE_APP_INFO);

    setTextScale(1);
    drawStringF(0, 60,  "Title:       %s", g_appTitle);
    drawStringF(0, 82,  "STitle:      %s", g_appSTitle);
    drawStringF(0, 104, "ContentID:   %s", g_appContent);
    drawStringF(0, 126, "TitleID:     %s [ %s ]", g_appTitleID, g_appRegion);
    drawStringF(0, 148, "Module:      %s", g_appModuleInfo.name);
    drawStringF(0, 170, "Module path: %s", g_appSceModuleInfo.path);
    drawStringF(0, 192, "Module NID:  0x%08X", g_appModuleInfo.module_nid);
}

void setupAppInfoMenu() {
    g_appModuleInfo.size = sizeof(tai_module_info_t);
    taiGetModuleInfo(TAI_MAIN_MODULE, &g_appModuleInfo);

    g_appSceModuleInfo.size = sizeof(SceKernelModuleInfo);
    sceKernelGetModuleInfo(g_appModuleInfo.modid, &g_appSceModuleInfo);

    sceAppMgrAppParamGetString(0, 6, g_appContent, 256);
    sceAppMgrAppParamGetString(0, 9, g_appTitle, 256);
    sceAppMgrAppParamGetString(0, 10, g_appSTitle, 64);
    sceAppMgrAppParamGetString(0, 12, g_appTitleID, 12);

    if (!strncmp(g_appTitleID, "PCSA", 4)) {
        snprintf(g_appRegion, 10, "US 1st");
    } else if (!strncmp(g_appTitleID, "PCSE", 4)) {
        snprintf(g_appRegion, 10, "US 3rd");
    } else if (!strncmp(g_appTitleID, "PCSB", 4)) {
        snprintf(g_appRegion, 10, "EU 1st");
    } else if (!strncmp(g_appTitleID, "PCSF", 4)) {
        snprintf(g_appRegion, 10, "EU 3rd");
    } else if (!strncmp(g_appTitleID, "PCSC", 4)) {
        snprintf(g_appRegion, 10, "JP 1st");
    } else if (!strncmp(g_appTitleID, "PCSG", 4)) {
        snprintf(g_appRegion, 10, "JP 3rd");
    } else if (!strncmp(g_appTitleID, "PCSD", 4)) {
        snprintf(g_appRegion, 10, "ASIA 1st");
    } else if (!strncmp(g_appTitleID, "PCSH", 4)) {
        snprintf(g_appRegion, 10, "ASIA 3rd");
    } else {
        snprintf(g_appRegion, 10, "?");
    }
}
