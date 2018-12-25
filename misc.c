#include <vitasdk.h>
#include <taihen.h>

#include "osd.h"
#include "main.h"

static uint8_t sceCtrlPeekBufferPositive_used = 0;
static uint8_t sceCtrlPeekBufferNegative_used = 0;
static uint8_t sceCtrlReadBufferPositive_used = 0;
static uint8_t sceCtrlReadBufferNegative_used = 0;

static uint8_t sceMotionGetState_used = 0;
static uint8_t sceMotionGetSensorState_used = 0;
static uint8_t sceMotionGetBasicOrientation_used = 0;

static uint8_t sceTouchPeek_front = 0;
static uint8_t sceTouchPeek_back = 0;
static uint8_t sceTouchRead_front = 0;
static uint8_t sceTouchRead_back = 0;

static int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    sceCtrlPeekBufferPositive_used = 1;
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[13], port, pad_data, count);
}
static int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *pad_data, int count) {
    sceCtrlPeekBufferNegative_used = 1;
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[14], port, pad_data, count);
}
static int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    sceCtrlReadBufferPositive_used = 1;
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[15], port, pad_data, count);
}
static int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *pad_data, int count) {
    sceCtrlReadBufferNegative_used = 1;
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[16], port, pad_data, count);
}

static int sceMotionGetState_patched(SceMotionState *motionState) {
    sceMotionGetState_used = 1;
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[17], motionState);
}
static int sceMotionGetSensorState_patched(SceMotionSensorState *sensorState, int numRecords) {
    sceMotionGetSensorState_used = 1;
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[18], sensorState, numRecords);
}
static int sceMotionGetBasicOrientation_patched(SceFVector3 *basicOrientation) {
    sceMotionGetBasicOrientation_used = 1;
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[19], basicOrientation);
}

static int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    if (port == SCE_TOUCH_PORT_FRONT) {
        sceTouchPeek_front = 1;
    } else if (port == SCE_TOUCH_PORT_BACK) {
        sceTouchPeek_back = 1;
    }
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[20], port, pData, nBufs);
}
static int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    if (port == SCE_TOUCH_PORT_FRONT) {
        sceTouchRead_front = 1;
    } else if (port == SCE_TOUCH_PORT_BACK) {
        sceTouchRead_back = 1;
    }
    if (g_menuVisible)
        return 0;
    return TAI_CONTINUE(int, g_hookrefs[21], port, pData, nBufs);
}

void drawMiscMenu(const SceDisplayFrameBuf *pParam) {
    osdSetTextScale(2);
    osdDrawStringF((pParam->width / 2) - osdGetTextWidth(MENU_TITLE_MISC) / 2, 5, MENU_TITLE_MISC);

    osdSetTextScale(1);
    osdDrawStringF(10, 60, "Input handlers:");
    int x = 30, y = 82;
    if (sceCtrlPeekBufferPositive_used)
        osdDrawStringF(x, y += 22, "sceCtrlPeekBufferPositive()");
    if (sceCtrlPeekBufferNegative_used)
        osdDrawStringF(x, y += 22, "sceCtrlPeekBufferNegative()");
    if (sceCtrlReadBufferPositive_used)
        osdDrawStringF(x, y += 22, "sceCtrlReadBufferPositive()");
    if (sceCtrlReadBufferNegative_used)
        osdDrawStringF(x, y += 22, "sceCtrlReadBufferNegative()");
    if (sceMotionGetState_used)
        osdDrawStringF(x, y += 22, "sceMotionGetState()");
    if (sceMotionGetSensorState_used)
        osdDrawStringF(x, y += 22, "sceMotionGetSensorState()");
    if (sceMotionGetBasicOrientation_used)
        osdDrawStringF(x, y += 22, "sceMotionGetBasicOrientation()");
    if (sceTouchPeek_front)
        osdDrawStringF(x, y += 22, "sceTouchPeek( FRONT )");
    if (sceTouchPeek_back)
        osdDrawStringF(x, y += 22, "sceTouchPeek( BACK )");
    if (sceTouchRead_front)
        osdDrawStringF(x, y += 22, "sceTouchRead( FRONT )");
    if (sceTouchRead_back)
        osdDrawStringF(x, y += 22, "sceTouchRead( BACK )");
}

void setupMiscMenu() {

    g_hooks[13] = taiHookFunctionImport(
                    &g_hookrefs[13],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xA9C3CED6,
                    sceCtrlPeekBufferPositive_patched);
    g_hooks[14] = taiHookFunctionImport(
                    &g_hookrefs[14],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x104ED1A7,
                    sceCtrlPeekBufferNegative_patched);
    g_hooks[15] = taiHookFunctionImport(
                    &g_hookrefs[15],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x67E7AB83,
                    sceCtrlReadBufferPositive_patched);
    g_hooks[16] = taiHookFunctionImport(
                    &g_hookrefs[16],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x15F96FB0,
                    sceCtrlReadBufferNegative_patched);

    g_hooks[17] = taiHookFunctionImport(
                    &g_hookrefs[17],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xBDB32767,
                    sceMotionGetState_patched);
    g_hooks[18] = taiHookFunctionImport(
                    &g_hookrefs[18],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x47D679EA,
                    sceMotionGetSensorState_patched);
    g_hooks[19] = taiHookFunctionImport(
                    &g_hookrefs[19],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x4F28BFE0,
                    sceMotionGetBasicOrientation_patched);

    g_hooks[20] = taiHookFunctionImport(
                    &g_hookrefs[20],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0xFF082DF0,
                    sceTouchPeek_patched);
    g_hooks[21] = taiHookFunctionImport(
                    &g_hookrefs[21],
                    TAI_MAIN_MODULE,
                    TAI_ANY_LIBRARY,
                    0x169A1D58,
                    sceTouchRead_patched);
}
