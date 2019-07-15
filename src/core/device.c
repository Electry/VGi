#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

static uint8_t g_scePowerSetConfigurationMode_mode = 0;

static int scePowerSetConfigurationMode_patched(int mode) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_POWER_SET_CONFIGURATION_MODE], mode);
    if (ret != SCE_OK)
        return ret;

    g_scePowerSetConfigurationMode_mode = mode;
    return ret;
}

void drawDevice(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;

    osdDrawStringF(x, y, "Using Wireless Features: %s", scePowerGetUsingWireless() == SCE_TRUE ? "YES" : "NO");
    osdDrawStringF(x, y += osdGetLineHeight(), "Requested Power Configuration: %s",
            g_scePowerSetConfigurationMode_mode == 0 ? "Unknown" :
            (g_scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_A ? "MODE_A" :
            (g_scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_B ? "MODE_B" :
            (g_scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "MODE_C" : "???"))));
    if (g_scePowerSetConfigurationMode_mode != 0) {
        osdDrawStringF(x + 20, y += osdGetLineHeight() * 1.5f, "GPU clock:               %s", g_scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_A ? "Normal (166 MHz)" : "High (222 MHz)");
        osdDrawStringF(x + 20, y += osdGetLineHeight(),      "WLAN enabled:            %s", g_scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_B ? "NO" : "YES");
        osdDrawStringF(x + 20, y += osdGetLineHeight(),      "Camera disabled:         %s",
                g_scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "YES" : "NO");
        osdDrawStringF(x + 20, y += osdGetLineHeight(),      "Max Brightness limited:  %s",
                g_scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "YES" : "NO");
    }

    osdDrawStringF(x, y += osdGetLineHeight() * 2, "Reported Clocks (ScePower):");
    osdDrawStringF(x + 20, y += osdGetLineHeight() * 1.5f, "CPU:   %d MHz", scePowerGetArmClockFrequency());
    osdDrawStringF(x + 20, y += osdGetLineHeight(),      "GPU:   %d MHz", scePowerGetGpuClockFrequency());
    osdDrawStringF(x + 20, y += osdGetLineHeight(),      "BUS:   %d MHz", scePowerGetBusClockFrequency());
    osdDrawStringF(x + 20, y += osdGetLineHeight(),      "XBAR:  %d MHz", scePowerGetGpuXbarClockFrequency());
}

void setupDevice() {
    hookFunctionImport(0x3CE187B6, HOOK_SCE_POWER_SET_CONFIGURATION_MODE, scePowerSetConfigurationMode_patched);
}
