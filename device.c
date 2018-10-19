#include <vitasdk.h>
#include <taihen.h>

#include "display.h"
#include "main.h"

static uint8_t scePowerSetConfigurationMode_mode = 0;


static int scePowerSetConfigurationMode_patched(int mode) {
    scePowerSetConfigurationMode_mode = mode;
    return TAI_CONTINUE(int, g_hookrefs[26], mode);
}

void drawDeviceMenu(const SceDisplayFrameBuf *pParam) {
    setTextScale(2);
    drawStringF((pParam->width / 2) - getTextWidth(MENU_TITLE_DEVICE) / 2, 5, MENU_TITLE_DEVICE);

    setTextScale(1);
    int y = 60;

    drawStringF(20, y += 22, "Using Wireless Features: %s", scePowerGetUsingWireless() == SCE_TRUE ? "YES" : "NO");
    drawStringF(20, y += 22, "PowerConfiguration: %s",
            scePowerSetConfigurationMode_mode == 0 ? "Unknown" :
            (scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_A ? "MODE_A" :
            (scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_B ? "MODE_B" :
            (scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "MODE_C" : "???"))));
    if (scePowerSetConfigurationMode_mode != 0) {
        drawStringF(40, y += 22, "GPU clock: %s", scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_A ? "normal" : "high");
        drawStringF(40, y += 22, "WLAN enabled: %s", scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_B ? "NO" : "YES");
        drawStringF(40, y += 22, "Camera disabled: %s, Max Brightness limited: %s",
                scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "YES" : "NO",
                scePowerSetConfigurationMode_mode == SCE_POWER_CONFIGURATION_MODE_C ? "YES" : "NO");
    }

    drawStringF(20, y += 44, "Current Clocks:");
    drawStringF(40, y += 22, "CPU: %d MHz", scePowerGetArmClockFrequency());
    drawStringF(40, y += 22, "GPU: %d MHz", scePowerGetGpuClockFrequency());
    drawStringF(40, y += 22, "BUS: %d MHz", scePowerGetBusClockFrequency());
    drawStringF(40, y += 22, "XBAR: %d MHz", scePowerGetGpuXbarClockFrequency());
}

void setupDeviceMenu() {

    g_hooks[26] = taiHookFunctionImport(
                    &g_hookrefs[26],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x3CE187B6,
                    scePowerSetConfigurationMode_patched);
}
