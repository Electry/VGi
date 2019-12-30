#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

tai_module_info_t g_app_tai_info = {0};
SceKernelModuleInfo g_app_sce_info = {0};
char g_app_titleid[12] = {0};

static char g_app_content[256] = {0};
static char g_app_title[256] = {0};
static char g_app_stitle[64] = {0};
static char g_app_region[10] = {0};

static void region_from_titleid(const char *titleid, char *region, size_t len) {
    if (!strncmp(titleid, "PCSA", 4)) {
        snprintf(region, len, "US 1st");
    } else if (!strncmp(titleid, "PCSE", 4)) {
        snprintf(region, len, "US 3rd");
    } else if (!strncmp(titleid, "PCSB", 4)) {
        snprintf(region, len, "EU 1st");
    } else if (!strncmp(titleid, "PCSF", 4)) {
        snprintf(region, len, "EU 3rd");
    } else if (!strncmp(titleid, "PCSC", 4)) {
        snprintf(region, len, "JP 1st");
    } else if (!strncmp(titleid, "PCSG", 4)) {
        snprintf(region, len, "JP 3rd");
    } else if (!strncmp(titleid, "PCSD", 4)) {
        snprintf(region, len, "ASIA 1st");
    } else if (!strncmp(titleid, "PCSH", 4)) {
        snprintf(region, len, "ASIA 3rd");
    } else {
        snprintf(region, len, "?");
    }
}

void vgi_dump_appinfo() {
    char msg[2048];
    snprintf(msg, sizeof(msg),
            "App info:\n"
            "  Title:       '%s'\n"
            "  STitle:      '%s'\n"
            "  ContentID:   %s\n"
            "  TitleID:     %s [ %s ]\n"
            "  Module:      %s\n"
            "  Module path: %s\n"
            "  Module NID:  0x%08lX\n",
            g_app_title,
            g_app_stitle,
            g_app_content,
            g_app_titleid, g_app_region,
            g_app_tai_info.name,
            g_app_sce_info.path,
            g_app_tai_info.module_nid);
    vgi_cmd_send_msg(msg);
}

void vgi_draw_appinfo(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0), "Title:       %s", g_app_title);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 1), "STitle:      %s", g_app_stitle);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 2), "ContentID:   %s", g_app_content);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3), "TitleID:     %s [ %s ]", g_app_titleid, g_app_region);

    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 5), "Module:      %s", g_app_tai_info.name);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 6), "Module path: %s", g_app_sce_info.path);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 7), "Module NID:  0x%08X", g_app_tai_info.module_nid);
}

void vgi_setup_appinfo() {
    g_app_tai_info.size = sizeof(tai_module_info_t);
    taiGetModuleInfo(TAI_MAIN_MODULE, &g_app_tai_info);

    g_app_sce_info.size = sizeof(SceKernelModuleInfo);
    sceKernelGetModuleInfo(g_app_tai_info.modid, &g_app_sce_info);

    sceAppMgrAppParamGetString(0, 6, g_app_content, sizeof(g_app_content));
    sceAppMgrAppParamGetString(0, 9, g_app_title, sizeof(g_app_title));
    sceAppMgrAppParamGetString(0, 10, g_app_stitle, sizeof(g_app_stitle));
    sceAppMgrAppParamGetString(0, 12, g_app_titleid, sizeof(g_app_titleid));

    region_from_titleid(g_app_titleid, g_app_region, sizeof(g_app_region));
}
