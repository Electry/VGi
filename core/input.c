#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

static uint8_t sceCtrlPeekBufferPositive_used = 0;
static uint8_t sceCtrlPeekBufferNegative_used = 0;
static uint8_t sceCtrlReadBufferPositive_used = 0;
static uint8_t sceCtrlReadBufferNegative_used = 0;

static uint8_t sceCtrlPeekBufferPositive2_used = 0;
static uint8_t sceCtrlPeekBufferNegative2_used = 0;
static uint8_t sceCtrlReadBufferPositive2_used = 0;
static uint8_t sceCtrlReadBufferNegative2_used = 0;

static uint8_t sceMotionGetState_used = 0;
static uint8_t sceMotionGetSensorState_used = 0;
static uint8_t sceMotionGetBasicOrientation_used = 0;

static uint8_t sceTouchPeek_front = 0;
static uint8_t sceTouchPeek_back = 0;
static uint8_t sceTouchRead_front = 0;
static uint8_t sceTouchRead_back = 0;

static int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE],
                            port, pad_data, count);
    sceCtrlPeekBufferPositive_used = 1;

    if (g_visible && pad_data)
        pad_data->buttons = 0;

    return ret;
}
static int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE],
                            port, pad_data, count);
    sceCtrlPeekBufferNegative_used = 1;

    if (g_visible && pad_data)
        pad_data->buttons = 0;

    return ret;
}
static int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_READ_BUFFER_POSITIVE],
                            port, pad_data, count);
    sceCtrlReadBufferPositive_used = 1;

    if (g_visible && pad_data)
        pad_data->buttons = 0;

    return ret;
}
static int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE],
                            port, pad_data, count);
    sceCtrlReadBufferNegative_used = 1;

    if (g_visible && pad_data)
        pad_data->buttons = 0;

    return ret;
}

static int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE_2],
                            port, pad_data, count);
    sceCtrlPeekBufferPositive2_used = 1;

    if (g_visible && pad_data)
        pad_data->buttons = 0;

    return ret;
}
static int sceCtrlPeekBufferNegative2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE_2],
                            port, pad_data, count);
    sceCtrlPeekBufferNegative2_used = 1;

    if (g_visible && pad_data)
        pad_data->buttons = 0;

    return ret;
}
static int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_READ_BUFFER_POSITIVE_2],
                            port, pad_data, count);
    sceCtrlReadBufferPositive2_used = 1;

    if (g_visible && pad_data)
        pad_data->buttons = 0;

    return ret;
}
static int sceCtrlReadBufferNegative2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE_2],
                            port, pad_data, count);
    sceCtrlReadBufferNegative2_used = 1;

    if (g_visible && pad_data)
        pad_data->buttons = 0;

    return ret;
}

static int sceMotionGetState_patched(SceMotionState *motionState) {
    sceMotionGetState_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_MOTION_GET_STATE], motionState);
}
static int sceMotionGetSensorState_patched(SceMotionSensorState *sensorState, int numRecords) {
    sceMotionGetSensorState_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_MOTION_GET_SENSOR_STATE], sensorState, numRecords);
}
static int sceMotionGetBasicOrientation_patched(SceFVector3 *basicOrientation) {
    sceMotionGetBasicOrientation_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_MOTION_GET_BASIC_ORIENTATION], basicOrientation);
}

static int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_TOUCH_PEEK], port, pData, nBufs);
    if (port == SCE_TOUCH_PORT_FRONT) {
        sceTouchPeek_front = 1;
    } else if (port == SCE_TOUCH_PORT_BACK) {
        sceTouchPeek_back = 1;
    }

    if (g_visible && pData) {
        pData->reportNum = 0;
        memset(pData->report, 0, sizeof(SceTouchReport) * SCE_TOUCH_MAX_REPORT);
    }

    return ret;
}
static int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_TOUCH_READ], port, pData, nBufs);
    if (port == SCE_TOUCH_PORT_FRONT) {
        sceTouchRead_front = 1;
    } else if (port == SCE_TOUCH_PORT_BACK) {
        sceTouchRead_back = 1;
    }

    if (g_visible && pData) {
        pData->reportNum = 0;
        memset(pData->report, 0, sizeof(SceTouchReport) * SCE_TOUCH_MAX_REPORT);
    }

    return ret;
}

void drawInput(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Input polls:");
    
    x += 20;
    y += osdGetLineHeight() * 0.5f;

    if (sceCtrlPeekBufferPositive_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceCtrlPeekBufferPositive()");
    if (sceCtrlPeekBufferNegative_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceCtrlPeekBufferNegative()");
    if (sceCtrlReadBufferPositive_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceCtrlReadBufferPositive()");
    if (sceCtrlReadBufferNegative_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceCtrlReadBufferNegative()");
    if (sceCtrlPeekBufferPositive2_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceCtrlPeekBufferPositive2()");
    if (sceCtrlPeekBufferNegative2_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceCtrlPeekBufferNegative2()");
    if (sceCtrlReadBufferPositive2_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceCtrlReadBufferPositive2()");
    if (sceCtrlReadBufferNegative2_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceCtrlReadBufferNegative2()");
    if (sceMotionGetState_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceMotionGetState()");
    if (sceMotionGetSensorState_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceMotionGetSensorState()");
    if (sceMotionGetBasicOrientation_used)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceMotionGetBasicOrientation()");
    if (sceTouchPeek_front)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceTouchPeek( FRONT )");
    if (sceTouchPeek_back)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceTouchPeek( BACK )");
    if (sceTouchRead_front)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceTouchRead( FRONT )");
    if (sceTouchRead_back)
        osdDrawStringF(x, y += osdGetLineHeight(), "sceTouchRead( BACK )");
}

void setupInput() {
    hookFunctionImport(0xA9C3CED6, HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE, sceCtrlPeekBufferPositive_patched);
    hookFunctionImport(0x104ED1A7, HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE, sceCtrlPeekBufferNegative_patched);
    hookFunctionImport(0x67E7AB83, HOOK_SCE_CTRL_READ_BUFFER_POSITIVE, sceCtrlReadBufferPositive_patched);
    hookFunctionImport(0x15F96FB0, HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE, sceCtrlReadBufferNegative_patched);
    hookFunctionImport(0x15F81E8C, HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE_2, sceCtrlPeekBufferPositive2_patched);
    hookFunctionImport(0x81A89660, HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE_2, sceCtrlPeekBufferNegative2_patched);
    hookFunctionImport(0xC4226A3E, HOOK_SCE_CTRL_READ_BUFFER_POSITIVE_2, sceCtrlReadBufferPositive2_patched);
    hookFunctionImport(0x27A0C5FB, HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE_2, sceCtrlReadBufferNegative2_patched);
    hookFunctionImport(0xBDB32767, HOOK_SCE_MOTION_GET_STATE, sceMotionGetState_patched);
    hookFunctionImport(0x47D679EA, HOOK_SCE_MOTION_GET_SENSOR_STATE, sceMotionGetSensorState_patched);
    hookFunctionImport(0x4F28BFE0, HOOK_SCE_MOTION_GET_BASIC_ORIENTATION, sceMotionGetBasicOrientation_patched);
    hookFunctionImport(0xFF082DF0, HOOK_SCE_TOUCH_PEEK, sceTouchPeek_patched);
    hookFunctionImport(0x169A1D58, HOOK_SCE_TOUCH_READ, sceTouchRead_patched);
}
