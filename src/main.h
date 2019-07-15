#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <math.h>

#define VGi_VERSION  "v1.0"

#define BUTTONS_FAST_MOVE_DELAY 500000

// .DATA
#define MAX_RENDER_TARGETS         64
#define MAX_COLOR_SURFACES         128
#define MAX_DEPTH_STENCIL_SURFACES 16
#define MAX_TEXTURES               64
#define MAX_GXM_SCENES             64
#define MAX_REGION_CLIPS           64
#define MAX_VIEWPORTS              64
#define MAX_MEM_BLOCKS             256
#define MAX_THREADS                64
#define MAX_PROGRAM_PARAMETERS     128

typedef enum {
    // graphics.c
    HOOK_SCE_DISPLAY_GET_REFRESH_RATE,
    HOOK_SCE_DISPLAY_GET_VCOUNT,
    HOOK_SCE_DISPLAY_WAIT_VBLANK_START,
    HOOK_SCE_DISPLAY_WAIT_VBLANK_START_CB,
    HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI,
    HOOK_SCE_DISPLAY_WAIT_VBLANK_START_MULTI_CB,
    HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF,
    HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_CB,
    HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI,
    HOOK_SCE_DISPLAY_WAIT_SET_FRAME_BUF_MULTI_CB,
    HOOK_SCE_GXM_INITIALIZE,
    // graphics_rt.c
    HOOK_SCE_GXM_CREATE_RENDER_TARGET,
    HOOK_SCE_GXM_DESTROY_RENDER_TARGET,
    // graphics_cs.c
    HOOK_SCE_GXM_COLOR_SURFACE_INIT,
    // graphics_dss.c
    HOOK_SCE_GXM_DEPTH_STENCIL_SURFACE_INIT,
    // graphics_tex.c
    HOOK_SCE_GXM_TEXTURE_INIT_SWIZZLED,
    HOOK_SCE_GXM_TEXTURE_INIT_LINEAR,
    HOOK_SCE_GXM_TEXTURE_INIT_LINEAR_STRIDED,
    HOOK_SCE_GXM_TEXTURE_INIT_TILED,
    HOOK_SCE_GXM_TEXTURE_INIT_CUBE,
    // graphics_scenes.c
    HOOK_SCE_GXM_BEGIN_SCENE,
    HOOK_SCE_GXM_BEGIN_SCENE_EX,
    // graphics_misc.c
    HOOK_SCE_GXM_SET_REGION_CLIP,
    HOOK_SCE_GXM_SET_VIEWPORT,
    HOOK_SCE_GXM_SET_DEFAULT_REGION_CLIP_AND_VIEWPORT,
    // graphics_cg.c
    HOOK_SCE_GXM_PROGRAM_FIND_PARAMETER_BY_NAME,
    // memory.c
    HOOK_SCE_KERNEL_ALLOC_MEM_BLOCK,
    HOOK_SCE_KERNEL_FREE_MEM_BLOCK,
    // threads.c
    HOOK_SCE_KERNEL_CREATE_THREAD,
    HOOK_SCE_KERNEL_DELETE_THREAD,
    // input.c
    HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE,
    HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE,
    HOOK_SCE_CTRL_READ_BUFFER_POSITIVE,
    HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE,
    HOOK_SCE_CTRL_PEEK_BUFFER_POSITIVE_2,
    HOOK_SCE_CTRL_PEEK_BUFFER_NEGATIVE_2,
    HOOK_SCE_CTRL_READ_BUFFER_POSITIVE_2,
    HOOK_SCE_CTRL_READ_BUFFER_NEGATIVE_2,
    HOOK_SCE_MOTION_GET_STATE,
    HOOK_SCE_MOTION_GET_SENSOR_STATE,
    HOOK_SCE_MOTION_GET_BASIC_ORIENTATION,
    HOOK_SCE_TOUCH_PEEK,
    HOOK_SCE_TOUCH_READ,
    // device.c
    HOOK_SCE_POWER_SET_CONFIGURATION_MODE,
    // ----------
    HOOK_MAX
} VGi_Hooks;

typedef enum {
    SECTION_MENU = 0,
    SECTION_APP_INFO,
    SECTION_GRAPHICS,
    SECTION_GRAPHICS_RT,
    SECTION_GRAPHICS_CS,
    SECTION_GRAPHICS_DSS,
    SECTION_GRAPHICS_TEX,
    SECTION_GRAPHICS_SCENES,
    SECTION_GRAPHICS_MISC,
    SECTION_GRAPHICS_CG,
    SECTION_MEMORY,
    SECTION_THREADS,
    SECTION_INPUT,
    SECTION_DEVICE,
    SECTION_MAX
} VGi_Section;

extern uint8_t g_scroll;
extern uint8_t g_visible;

extern SceUID g_hooks[];
extern tai_hook_ref_t g_hookrefs[];

void hookFunctionImport(uint32_t nid, uint32_t id, const void *func);
void drawScrollIndicator(int x, int y, uint8_t scroll_down, int count);

void setupAppInfo();
void setupGraphics();
void setupGraphicsRT();
void setupGraphicsCS();
void setupGraphicsDSS();
void setupGraphicsTex();
void setupGraphicsScenes();
void setupGraphicsMisc();
void setupGraphicsCg();
void setupMemory();
void setupThreads();
void setupInput();
void setupDevice();

void drawAppInfo(int minX, int minY, int maxX, int maxY);
void drawGraphics(int minX, int minY, int maxX, int maxY, const SceDisplayFrameBuf *pParam);
void drawGraphicsRT(int minX, int minY, int maxX, int maxY);
void drawGraphicsCS(int minX, int minY, int maxX, int maxY);
void drawGraphicsDSS(int minX, int minY, int maxX, int maxY);
void drawGraphicsTex(int minX, int minY, int maxX, int maxY);
void drawGraphicsScenes(int minX, int minY, int maxX, int maxY);
void drawGraphicsMisc(int minX, int minY, int maxX, int maxY);
void drawGraphicsCg(int minX, int minY, int maxX, int maxY);
void drawMemory(int minX, int minY, int maxX, int maxY);
void drawThreads(int minX, int minY, int maxX, int maxY);
void drawInput(int minX, int minY, int maxX, int maxY);
void drawDevice(int minX, int minY, int maxX, int maxY);

void checkButtonsGraphics(uint32_t buttons_now, uint32_t buttons);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define SCE_POWER_CONFIGURATION_MODE_A 0x00000080U
#define SCE_POWER_CONFIGURATION_MODE_B 0x00000800U
#define SCE_POWER_CONFIGURATION_MODE_C 0x00010880U

#endif