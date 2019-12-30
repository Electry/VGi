#include <vitasdk.h>
#include <taihen.h>

#include "../gui.h"
#include "../main.h"

static uint8_t g_sceCtrlPeekBufferPositive_used = 0;
static uint8_t g_sceCtrlPeekBufferNegative_used = 0;
static uint8_t g_sceCtrlReadBufferPositive_used = 0;
static uint8_t g_sceCtrlReadBufferNegative_used = 0;

static uint8_t g_sceCtrlPeekBufferPositive2_used = 0;
static uint8_t g_sceCtrlPeekBufferNegative2_used = 0;
static uint8_t g_sceCtrlReadBufferPositive2_used = 0;
static uint8_t g_sceCtrlReadBufferNegative2_used = 0;

static uint8_t g_sceMotionGetState_used = 0;
static uint8_t g_sceMotionGetSensorState_used = 0;
static uint8_t g_sceMotionGetBasicOrientation_used = 0;

static uint8_t g_sceTouchPeek_front = 0;
static uint8_t g_sceTouchPeek_back = 0;
static uint8_t g_sceTouchRead_front = 0;
static uint8_t g_sceTouchRead_back = 0;

static int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE],
                            port, pad_data, count);
    g_sceCtrlPeekBufferPositive_used = 1;

    if (g_visible && pad_data)
        for (int i = 0; i < ret; i++)
            pad_data[i].buttons = 0;

    return ret;
}

static int sceCtrlPeekBufferNegative_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE],
                            port, pad_data, count);
    g_sceCtrlPeekBufferNegative_used = 1;

    if (g_visible && pad_data)
        for (int i = 0; i < ret; i++)
            pad_data[i].buttons = 0;

    return ret;
}

static int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_READ_BUFFER_POSITIVE],
                            port, pad_data, count);
    g_sceCtrlReadBufferPositive_used = 1;

    if (g_visible && pad_data)
        for (int i = 0; i < ret; i++)
            pad_data[i].buttons = 0;

    return ret;
}

static int sceCtrlReadBufferNegative_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE],
                            port, pad_data, count);
    g_sceCtrlReadBufferNegative_used = 1;

    if (g_visible && pad_data)
        for (int i = 0; i < ret; i++)
            pad_data[i].buttons = 0;

    return ret;
}

static int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE_2],
                            port, pad_data, count);
    g_sceCtrlPeekBufferPositive2_used = 1;

    if (g_visible && pad_data)
        for (int i = 0; i < ret; i++)
            pad_data[i].buttons = 0;

    return ret;
}

static int sceCtrlPeekBufferNegative2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE_2],
                            port, pad_data, count);
    g_sceCtrlPeekBufferNegative2_used = 1;

    if (g_visible && pad_data)
        for (int i = 0; i < ret; i++)
            pad_data[i].buttons = 0;

    return ret;
}

static int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_READ_BUFFER_POSITIVE_2],
                            port, pad_data, count);
    g_sceCtrlReadBufferPositive2_used = 1;

    if (g_visible && pad_data)
        for (int i = 0; i < ret; i++)
            pad_data[i].buttons = 0;

    return ret;
}

static int sceCtrlReadBufferNegative2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE_2],
                            port, pad_data, count);
    g_sceCtrlReadBufferNegative2_used = 1;

    if (g_visible && pad_data)
        for (int i = 0; i < ret; i++)
            pad_data[i].buttons = 0;

    return ret;
}

static int sceMotionGetState_patched(SceMotionState *motionState) {
    g_sceMotionGetState_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_MOTION_GET_STATE], motionState);
}

static int sceMotionGetSensorState_patched(SceMotionSensorState *sensorState, int numRecords) {
    g_sceMotionGetSensorState_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_MOTION_GET_SENSOR_STATE], sensorState, numRecords);
}

static int sceMotionGetBasicOrientation_patched(SceFVector3 *basicOrientation) {
    g_sceMotionGetBasicOrientation_used = 1;
    return TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_MOTION_GET_BASIC_ORIENTATION], basicOrientation);
}

static int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_TOUCH_PEEK], port, pData, nBufs);
    if (port == SCE_TOUCH_PORT_FRONT) {
        g_sceTouchPeek_front = 1;
    } else if (port == SCE_TOUCH_PORT_BACK) {
        g_sceTouchPeek_back = 1;
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
        g_sceTouchRead_front = 1;
    } else if (port == SCE_TOUCH_PORT_BACK) {
        g_sceTouchRead_back = 1;
    }

    if (g_visible && pData) {
        pData->reportNum = 0;
        memset(pData->report, 0, sizeof(SceTouchReport) * SCE_TOUCH_MAX_REPORT);
    }

    return ret;
}

void vgi_draw_input(int xoff, int yoff, int x2off, int y2off) {
    vgi_gui_print(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0), "Input polls:");

    int x = GUI_ANCHOR_LX(xoff + 20, 0);
    int y = GUI_ANCHOR_TY(yoff, 0.5f);

    if (g_sceCtrlPeekBufferPositive_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceCtrlPeekBufferPositive()");
    if (g_sceCtrlPeekBufferNegative_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceCtrlPeekBufferNegative()");
    if (g_sceCtrlReadBufferPositive_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceCtrlReadBufferPositive()");
    if (g_sceCtrlReadBufferNegative_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceCtrlReadBufferNegative()");
    if (g_sceCtrlPeekBufferPositive2_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceCtrlPeekBufferPositive2()");
    if (g_sceCtrlPeekBufferNegative2_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceCtrlPeekBufferNegative2()");
    if (g_sceCtrlReadBufferPositive2_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceCtrlReadBufferPositive2()");
    if (g_sceCtrlReadBufferNegative2_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceCtrlReadBufferNegative2()");
    if (g_sceMotionGetState_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceMotionGetState()");
    if (g_sceMotionGetSensorState_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceMotionGetSensorState()");
    if (g_sceMotionGetBasicOrientation_used)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceMotionGetBasicOrientation()");
    if (g_sceTouchPeek_front)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceTouchPeek( FRONT )");
    if (g_sceTouchPeek_back)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceTouchPeek( BACK )");
    if (g_sceTouchRead_front)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceTouchRead( FRONT )");
    if (g_sceTouchRead_back)
        vgi_gui_printf(x, y += GUI_FONT_H, "sceTouchRead( BACK )");
}

void vgi_setup_input() {
    HOOK_FUNCTION_IMPORT(0xA9C3CED6, HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE, sceCtrlPeekBufferPositive_patched);
    HOOK_FUNCTION_IMPORT(0x104ED1A7, HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE, sceCtrlPeekBufferNegative_patched);
    HOOK_FUNCTION_IMPORT(0x67E7AB83, HOOK_SCE_CTRL_READ_BUFFER_POSITIVE, sceCtrlReadBufferPositive_patched);
    HOOK_FUNCTION_IMPORT(0x15F96FB0, HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE, sceCtrlReadBufferNegative_patched);
    HOOK_FUNCTION_IMPORT(0x15F81E8C, HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE_2, sceCtrlPeekBufferPositive2_patched);
    HOOK_FUNCTION_IMPORT(0x81A89660, HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE_2, sceCtrlPeekBufferNegative2_patched);
    HOOK_FUNCTION_IMPORT(0xC4226A3E, HOOK_SCE_CTRL_READ_BUFFER_POSITIVE_2, sceCtrlReadBufferPositive2_patched);
    HOOK_FUNCTION_IMPORT(0x27A0C5FB, HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE_2, sceCtrlReadBufferNegative2_patched);
    HOOK_FUNCTION_IMPORT(0xBDB32767, HOOK_SCE_MOTION_GET_STATE, sceMotionGetState_patched);
    HOOK_FUNCTION_IMPORT(0x47D679EA, HOOK_SCE_MOTION_GET_SENSOR_STATE, sceMotionGetSensorState_patched);
    HOOK_FUNCTION_IMPORT(0x4F28BFE0, HOOK_SCE_MOTION_GET_BASIC_ORIENTATION, sceMotionGetBasicOrientation_patched);
    HOOK_FUNCTION_IMPORT(0xFF082DF0, HOOK_SCE_TOUCH_PEEK, sceTouchPeek_patched);
    HOOK_FUNCTION_IMPORT(0x169A1D58, HOOK_SCE_TOUCH_READ, sceTouchRead_patched);
}
