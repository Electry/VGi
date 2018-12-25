#include <vitasdk.h>
#include <taihen.h>

#include "osd.h"
#include "main.h"

static uint8_t scePowerSetConfigurationMode_mode = 0;


static int scePowerSetConfigurationMode_patched(int mode) {
    scePowerSetConfigurationMode_mode = mode;
    return TAI_CONTINUE(int, g_hookrefs[26], mode);
}

void drawDeviceMenu(const SceDisplayFrameBuf *pParam) {
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_DEVICE) / 2, 5, MENU_TITLE_DEVICE);

    osdSetTextScale(1);
    int x = 10, y = 60;

    osdDrawStringF(x, y += 22, "Using Wireless Features: %s", scePowerGetUsingWireless() == SCE_TRUE ? "YES" : "NO");
    osdDrawStringF(x, y += 22, "PowerConfiguration: %s",
            scePowerSetConfigurationMode_mode == 0 ? "Unknown" :
            (scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_A ? "MODE_A" :
            (scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_B ? "MODE_B" :
            (scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "MODE_C" : "???"))));
    if (scePowerSetConfigurationMode_mode != 0) {
        osdDrawStringF(x + 20, y += 22, "GPU clock: %s", scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_A ? "normal" : "high");
        osdDrawStringF(x + 20, y += 22, "WLAN enabled: %s", scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_B ? "NO" : "YES");
        osdDrawStringF(x + 20, y += 22, "Camera disabled: %s, Max Brightness limited: %s",
                scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "YES" : "NO",
                scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "YES" : "NO");
    }

    osdDrawStringF(x, y += 44, "Current Clocks:");
    osdDrawStringF(x + 20, y += 22, "CPU: %d MHz", scePowerGetArmClockFrequency());
    osdDrawStringF(x + 20, y += 22, "GPU: %d MHz", scePowerGetGpuClockFrequency());
    osdDrawStringF(x + 20, y += 22, "BUS: %d MHz", scePowerGetBusClockFrequency());
    osdDrawStringF(x + 20, y += 22, "XBAR: %d MHz", scePowerGetGpuXbarClockFrequency());
}

void setupDeviceMenu() {

    g_hooks[26] = taiHookFunctionImport(
                    &g_hookrefs[26],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x3CE187B6,
                    scePowerSetConfigurationMode_patched);
}
