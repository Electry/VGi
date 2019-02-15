#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

static tai_module_info_t g_appModuleInfo = {0};
static SceKernelModuleInfo g_appSceModuleInfo = {0};
static char g_appContent[256] = {0};
static char g_appTitle[256] = {0};
static char g_appSTitle[64] = {0};
static char g_appTitleID[12] = {0};
static char g_appRegion[10] = {0};

static void regionFromTitleID() {
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

void drawAppInfo(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;

    osdDrawStringF(x, y,                       "Title:       %s", g_appTitle);
    osdDrawStringF(x, y += osdGetLineHeight(), "STitle:      %s", g_appSTitle);
    osdDrawStringF(x, y += osdGetLineHeight(), "ContentID:   %s", g_appContent);
    osdDrawStringF(x, y += osdGetLineHeight(), "TitleID:     %s [ %s ]", g_appTitleID, g_appRegion);

    y += osdGetLineHeight();
    osdDrawStringF(x, y += osdGetLineHeight(), "Module:      %s", g_appModuleInfo.name);
    osdDrawStringF(x, y += osdGetLineHeight(), "Module path: %s", g_appSceModuleInfo.path);
    osdDrawStringF(x, y += osdGetLineHeight(), "Module NID:  0x%08X", g_appModuleInfo.module_nid);
}

void setupAppInfo() {
    g_appModuleInfo.size = sizeof(tai_module_info_t);
    taiGetModuleInfo(TAI_MAIN_MODULE, &g_appModuleInfo);

    g_appSceModuleInfo.size = sizeof(SceKernelModuleInfo);
    sceKernelGetModuleInfo(g_appModuleInfo.modid, &g_appSceModuleInfo);

    sceAppMgrAppParamGetString(0, 6, g_appContent, 256);
    sceAppMgrAppParamGetString(0, 9, g_appTitle, 256);
    sceAppMgrAppParamGetString(0, 10, g_appSTitle, 64);
    sceAppMgrAppParamGetString(0, 12, g_appTitleID, 12);

    regionFromTitleID();
}
