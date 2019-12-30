#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

typedef struct {
    bool arm_high;
    bool gpu_high;
    bool xbar_high;
    bool wlan_on;
    bool camera_on;
    bool brightness_limited;
} vgi_power_mode_opts_t;

static int g_power_mode = 0;

static int scePowerSetConfigurationMode_patched(int mode) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_POWER_SET_CONFIGURATION_MODE], mode);
    if (ret != SCE_OK)
        return ret;

    g_power_mode = mode;
    return ret;
}

static const char *get_power_mode_text(int mode) {
    switch (mode) {
        default:      return "Unknown";
        case 0:       return "Default (A)";
        // arm=333, gpu=111, xbar=111, wlan=on,  camera=on,  brightness=100
        case 0x80:    return "A";
        // arm=333, gpu=166, xbar=166, wlan=off, camera=on,  brightness=100
        case 0x800:   return "B";
        // arm=333, gpu=166, xbar=166, wlan=on,  camera=off, brightness=77
        case 0x10880: return "C";
        // arm=333, gpu=111, xbar=166, wlan=on,  camera=on,  brightness=100
        case 0x1080:  return "X";
        // arm=444, gpu=166, xbar=166, wlan=off, camera=on,  brightness=100
        case 0x10080: return "Y";
        // arm=444, gpu=166, xbar=166, wlan=on,  camera=n/a, brightness=n/a
        case 0x10888: return "Z (PSTV only)";
    }
}

static void get_power_mode_opts(int mode, vgi_power_mode_opts_t *opts) {
    opts->arm_high = false;
    opts->gpu_high = false;
    opts->xbar_high = false;
    opts->wlan_on = false;
    opts->camera_on = false;
    opts->brightness_limited = false;

    switch (mode) {
        case 0:
        case 0x80: opts->wlan_on = opts->camera_on = true; break;
        case 0x800: opts->gpu_high = opts->xbar_high = opts->camera_on = true; break;
        case 0x10880: opts->gpu_high = opts->xbar_high = opts->wlan_on
                        = opts->brightness_limited = true; break;
        case 0x1080: opts->xbar_high = opts->wlan_on = opts->camera_on = true; break;
        case 0x10080: opts->arm_high = opts->gpu_high = opts->xbar_high
                        = opts->camera_on = true; break;
        case 0x10888: opts->arm_high = opts->gpu_high = opts->xbar_high = true; break;
    }
}

void vgi_dump_device() {
    char msg[2048];
    vgi_power_mode_opts_t opts;
    get_power_mode_opts(g_power_mode, &opts);

    snprintf(msg, sizeof(msg),
            "Device info:\n"
            "  Wireless Features Enabled: %s\n"
            "  Requested Power Configuration Mode: %s (0x%X)\n"
            "    CPU clock:           %s\n"
            "    GPU clock:           %s\n"
            "    GPU Crossbar clock:  %s\n"
            "    WLAN enabled:        %s\n"
            "    Camera allowed:      %s\n"
            "    Brightness limited:  %s\n"
            "  Reported Clocks (ScePower):\n"
            "    CPU:           %d MHz\n"
            "    GPU:           %d MHz\n"
            "    BUS:           %d MHz\n"
            "    GPU Crossbar:  %d MHz\n",
            scePowerGetUsingWireless() == SCE_TRUE ? "YES" : "NO",
            get_power_mode_text(g_power_mode), g_power_mode,
            opts.arm_high ? "High (444 MHz)" : "Normal (333 MHz)",
            opts.gpu_high ? "High (166 MHz)" : "Normal (111 MHz)",
            opts.xbar_high ? "High (166 MHz)" : "Normal (111 MHz)",
            opts.wlan_on ? "YES" : "NO",
            opts.camera_on ? "YES" : "NO",
            opts.brightness_limited ? "YES" : "NO",
            scePowerGetArmClockFrequency(),
            scePowerGetGpuClockFrequency(),
            scePowerGetBusClockFrequency(),
            scePowerGetGpuXbarClockFrequency());
    vgi_cmd_send_msg(msg);
}

void vgi_draw_device(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Wireless Features Enabled: %s",
            scePowerGetUsingWireless() == SCE_TRUE ? "YES" : "NO");

    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 2),
            "Requested Power Configuration Mode: %s (0x%X)",
            get_power_mode_text(g_power_mode),
            g_power_mode);

    vgi_power_mode_opts_t opts;
    get_power_mode_opts(g_power_mode, &opts);

    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 3), "CPU clock:           %s",
            opts.arm_high ? "High (444 MHz)" : "Normal (333 MHz)");
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 4), "GPU clock:           %s",
            opts.gpu_high ? "High (166 MHz)" : "Normal (111 MHz)");
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 5), "GPU Crossbar clock:  %s",
            opts.xbar_high ? "High (166 MHz)" : "Normal (111 MHz)");
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 6), "WLAN enabled:        %s",
            opts.wlan_on ? "YES" : "NO");
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 7), "Camera allowed:      %s",
            opts.camera_on ? "YES" : "NO");
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 8), "Brightness limited:  %s",
            opts.brightness_limited ? "YES" : "NO");

    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 10),
            "Reported Clocks (ScePower):");
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 11),
            "CPU:           %d MHz", scePowerGetArmClockFrequency());
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 12),
            "GPU:           %d MHz", scePowerGetGpuClockFrequency());
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 13),
            "BUS:           %d MHz", scePowerGetBusClockFrequency());
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 20, 0), GUI_ANCHOR_TY(yoff, 14),
            "GPU Crossbar:  %d MHz", scePowerGetGpuXbarClockFrequency());
}

void vgi_setup_device() {
    HOOK_FUNCTION_IMPORT(0x3CE187B6, HOOK_SCE_POWER_SET_CONFIGURATION_MODE, scePowerSetConfigurationMode_patched);
}
