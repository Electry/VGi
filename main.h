#ifndef _MAIN_H_
#define _MAIN_H_

#define MENU_TITLE_APP_INFO   "App Info"
#define MENU_TITLE_GRAPHICS   "Graphics"
#define MENU_TITLE_GRAPHICS_2 "Render Targets"
#define MENU_TITLE_GRAPHICS_3 "Color Surfaces"
#define MENU_TITLE_MEMORY     "Memory"
#define MENU_TITLE_MISC       "Misc"
#define MENU_TITLE_DEVICE     "Device"

typedef enum {
    MENU_APP_INFO,
    MENU_GRAPHICS,
    MENU_GRAPHICS_2,
    MENU_GRAPHICS_3,
    MENU_MEMORY,
    MENU_MISC,
    MENU_DEVICE,
    MENU_MAX
} VGi_MenuSection;

extern uint8_t g_menuScroll;

extern SceUID g_hooks[];
extern tai_hook_ref_t g_hookrefs[];

void setupAppInfoMenu();
void drawAppInfoMenu(const SceDisplayFrameBuf *pParam);

void setupGraphicsMenu();
void drawGraphicsMenu(const SceDisplayFrameBuf *pParam);
void drawGraphics2Menu(const SceDisplayFrameBuf *pParam);
void drawGraphics3Menu(const SceDisplayFrameBuf *pParam);

void setupMemoryMenu();
void drawMemoryMenu(const SceDisplayFrameBuf *pParam);

void setupMiscMenu();
void drawMiscMenu(const SceDisplayFrameBuf *pParam);

#endif