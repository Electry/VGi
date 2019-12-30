#ifndef _GUI_H_
#define _GUI_H_
#include <stdbool.h>

// scaling done during drawing
#define GUI_WIDTH  960
#define GUI_HEIGHT 544
#define GUI_FONT_W 12
#define GUI_FONT_H 24

#define GUI_ANCHOR_LX(off, len) (off + (len) * GUI_FONT_W)
#define GUI_ANCHOR_RX(off, len) (GUI_WIDTH - (off) - (len) * GUI_FONT_W)
#define GUI_ANCHOR_RX2(off, len, scale) (GUI_WIDTH - (off) - (len) * GUI_FONT_W * (scale))

#define GUI_ANCHOR_TY(off, lines) (off + (lines) * GUI_FONT_H)
#define GUI_ANCHOR_BY(off, lines) (GUI_HEIGHT - (off) - (lines) * GUI_FONT_H)
#define GUI_ANCHOR_BY2(off, lines, scale) (GUI_HEIGHT - (off) - (lines) * GUI_FONT_H * (scale))

#define GUI_ANCHOR_CX(len) (GUI_WIDTH / 2 - ((len) * GUI_FONT_W) / 2)
#define GUI_ANCHOR_CX2(len, scale) (GUI_WIDTH / 2 - ((len) * GUI_FONT_W * (scale)) / 2)
#define GUI_ANCHOR_CY(lines) (GUI_HEIGHT / 2 - ((lines) * GUI_FONT_H) / 2)

typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    } rgba;
    uint32_t uint32;
} rgba_t;

extern SceDisplayFrameBuf g_gui_framebuf;

void vgi_gui_set_framebuf(const SceDisplayFrameBuf *param);
void vgi_gui_set_back_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void vgi_gui_set_text_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void vgi_gui_set_text_scale(float scale);

void vgi_gui_clear_screen();
void vgi_gui_print(int x, int y, const char *str);
void vgi_gui_printf(int x, int y, const char *format, ...);

void vgi_gui_draw_scrollbar(int x, int y, bool scroll_down, int count);

#define GUI_SCROLLABLE(max_items, cur_items, lines_per_item, x, y, x2, y2)               \
    int max_fit_##cur_items##_ = ((y2) - (y) - (lines_per_item) * GUI_FONT_H)            \
                            / ((lines_per_item) * GUI_FONT_H);                           \
    if (g_scroll > 0) {                                                                  \
        vgi_gui_draw_scrollbar((x2) - 2 * GUI_FONT_W, (y) + 1 * GUI_FONT_H,              \
                                false, g_scroll);                                        \
    }                                                                                    \
    if (cur_items >= g_scroll + max_fit_##cur_items##_) {                                \
        vgi_gui_draw_scrollbar((x2) - 2 * GUI_FONT_W, (y2) - 2 * GUI_FONT_H,             \
                                true, (cur_items) - g_scroll - max_fit_##cur_items##_);  \
    }                                                                                    \
    for (int i = g_scroll; i < MIN(g_scroll + max_fit_##cur_items##_, (max_items)); i++)

#endif
