#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <math.h>

#define VGi_VERSION  "v2.0"

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
#define MAX_PROGRAMS               128
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
    HOOK_SCE_GXM_RESERVE_VERTEX_DEFAULT_UNIFORM_BUFFER,
    HOOK_SCE_GXM_RESERVE_FRAGMENT_DEFAULT_UNIFORM_BUFFER,
    HOOK_SCE_GXM_SET_VERTEX_DEFAULT_UNIFORM_BUFFER,
    HOOK_SCE_GXM_SET_FRAGMENT_DEFAULT_UNIFORM_BUFFER,
    HOOK_SCE_GXM_SET_VERTEX_UNIFORM_BUFFER,
    HOOK_SCE_GXM_SET_FRAGMENT_UNIFORM_BUFFER,
    HOOK_SCE_GXM_END_SCENE,
    HOOK_SCE_GXM_SET_VERTEX_PROGRAM,
    HOOK_SCE_GXM_SET_FRAGMENT_PROGRAM,
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
} vgi_hook_t;

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
} vgi_section_t;

// core/memory.c
typedef struct {
    uint8_t success;
    uint8_t active;
    char name[128];
    SceKernelMemBlockType type;
    int size;
    SceUID ret;
    void *base;
} vgi_memblock_t;

#define HOOK_FUNCTION_IMPORT(nid, id, func)                                   \
    g_hooks[(id)] = taiHookFunctionImport(&g_hookrefs[(id)], TAI_MAIN_MODULE, \
                                        TAI_ANY_LIBRARY, (nid), (func));

extern uint8_t g_scroll;
extern uint8_t g_visible;

extern SceUID g_hooks[];
extern tai_hook_ref_t g_hookrefs[];

extern tai_module_info_t g_app_tai_info;
extern SceKernelModuleInfo g_app_sce_info;
extern char g_app_titleid[];

void vgi_setup_appinfo();
void vgi_setup_graphics();
void vgi_setup_graphics_rt();
void vgi_setup_graphics_cs();
void vgi_setup_graphics_dss();
void vgi_setup_graphics_tex();
void vgi_setup_graphics_scenes();
void vgi_setup_graphics_misc();
void vgi_setup_graphics_cg();
void vgi_setup_memory();
void vgi_setup_threads();
void vgi_setup_input();
void vgi_setup_device();

void vgi_draw_appinfo(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_graphics(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_graphics_rt(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_graphics_cs(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_graphics_dss(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_graphics_tex(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_graphics_scenes(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_graphics_misc(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_graphics_cg(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_memory(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_threads(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_input(int xoff, int yoff, int x2off, int y2off);
void vgi_draw_device(int xoff, int yoff, int x2off, int y2off);

void vgi_check_buttons_graphics(uint32_t buttons_now, uint32_t buttons);

const char *vgi_get_memblock_type_text(SceKernelMemBlockType type);
vgi_memblock_t *vgi_get_memblocks();

void vgi_dump_appinfo();
void vgi_dump_graphics();
void vgi_dump_graphics_rt();
void vgi_dump_graphics_cs();
void vgi_dump_graphics_dss();
void vgi_dump_graphics_tex();
void vgi_dump_graphics_scenes();
void vgi_dump_graphics_misc();
void vgi_dump_graphics_cg();
void vgi_dump_graphics_unibuf(const SceGxmProgram *program);
void vgi_dump_device();

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#endif
