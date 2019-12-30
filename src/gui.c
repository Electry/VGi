#include <vitasdk.h>
#include <stdbool.h>

#include "gui.h"
#include "gui_font_ter-u14b.h"
#include "gui_font_ter-u18b.h"
#include "gui_font_ter-u24b.h"

int vsnprintf(char *s, size_t n, const char *format, va_list arg);

SceDisplayFrameBuf g_gui_framebuf;

static const unsigned char *g_font;
static unsigned char        g_font_width;
static unsigned char        g_font_height;

static float  g_font_scale = 1;
static rgba_t g_color_text = {.rgba = {255, 255, 255, 255}};
static rgba_t g_color_bg   = {.rgba = {  0,   0,   0, 255}};

static rgba_t blend_color(rgba_t fg, rgba_t bg) {
    uint8_t inv_alpha = 255 - fg.rgba.a;

    rgba_t result;
    result.rgba.r = ((fg.rgba.a * fg.rgba.r + inv_alpha * bg.rgba.r) >> 8); // R
    result.rgba.g = ((fg.rgba.a * fg.rgba.g + inv_alpha * bg.rgba.g) >> 8); // G
    result.rgba.b = ((fg.rgba.a * fg.rgba.b + inv_alpha * bg.rgba.b) >> 8); // B
    result.rgba.a = 0xFF; // A
    return result;
}

static void draw_character(const char character, int x, int y) {
    for (int yy = 0; yy < g_font_height * g_font_scale; yy++) {
        int yy_font = yy / g_font_scale;

        uint32_t displacement = x + (y + yy) * g_gui_framebuf.pitch;
        if (displacement >= g_gui_framebuf.pitch * g_gui_framebuf.height)
            return; // out of bounds

        rgba_t *px = (rgba_t *)g_gui_framebuf.base + displacement;

        for (int xx = 0; xx < g_font_width * g_font_scale; xx++) {
            if (x + xx >= g_gui_framebuf.width)
                return; // out of bounds

            // Get px 0/1 from font.h
            int xx_font = xx / g_font_scale;
            uint32_t char_pos = character * (g_font_height * (((g_font_width - 1) / 8) + 1));
            uint32_t char_posh = char_pos + (yy_font * (((g_font_width - 1) / 8) + 1));
            uint8_t char_byte = g_font[char_posh + (xx_font / 8)];

            rgba_t color = ((char_byte >> (7 - (xx_font % 8))) & 1) ? g_color_text : g_color_bg;

            if (color.rgba.a > 0) {
                if (color.rgba.a < 255) {
                    px[xx] = blend_color(color, px[xx]);
                } else {
                    px[xx] = color;
                }
            }
        }
    }
}

void vgi_gui_set_framebuf(const SceDisplayFrameBuf *pParam) {
    memcpy(&g_gui_framebuf, pParam, sizeof(SceDisplayFrameBuf));

    // Set appropriate font
    if (pParam->width <= 640) {
        // 640x368 - Terminus 8x14 Bold
        g_font = FONT_TER_U14B;
        g_font_width = 8;
        g_font_height = 14;
    } else if (pParam->width <= 720) {
        // 720x408 - Terminus 10x18 Bold
        g_font = FONT_TER_U18B;
        g_font_width = 9; // <- trim last col, better scaling
        g_font_height = 18;
    } else {
        // 960x544 - Terminus 12x24 Bold
        g_font = FONT_TER_U24B;
        g_font_width = 12;
        g_font_height = 24;
    }
}

void vgi_gui_set_text_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_color_text.rgba.r = r;
    g_color_text.rgba.g = g;
    g_color_text.rgba.b = b;
    g_color_text.rgba.a = a;
}

void vgi_gui_set_back_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_color_bg.rgba.r = r;
    g_color_bg.rgba.g = g;
    g_color_bg.rgba.b = b;
    g_color_bg.rgba.a = a;
}

void vgi_gui_set_text_scale(float scale) {
    g_font_scale = scale;
}

void vgi_gui_clear_screen() {
    if (g_color_bg.rgba.a == 255) {
        memset(g_gui_framebuf.base, 0, sizeof(uint32_t) * (g_gui_framebuf.pitch * g_gui_framebuf.height));
    } else {
        for (int y = 0; y < g_gui_framebuf.height - 1; y += 2) {
            for (int x = 0; x < g_gui_framebuf.width; x += 2) {
                rgba_t *px = (rgba_t *)g_gui_framebuf.base + y * g_gui_framebuf.pitch + x;
                rgba_t color = blend_color(g_color_bg, *px);

                px[0] = color;
                px[1] = color;
                px[g_gui_framebuf.pitch] = color;
                px[g_gui_framebuf.pitch + 1] = color;
            }
        }
    }
}

void vgi_gui_print(int x, int y, const char *str) {
    x = (int)(x * (g_gui_framebuf.width / 960.0f));
    y = (int)(y * (g_gui_framebuf.height / 544.0f));

    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++)
        draw_character(str[i], x + i * g_font_width * g_font_scale, y);
}

void vgi_gui_printf(int x, int y, const char *format, ...) {
    char buffer[512];
    va_list va;

    va_start(va, format);
    vsnprintf(buffer, sizeof(buffer), format, va);
    va_end(va);

    vgi_gui_print(x, y, buffer);
}

void vgi_gui_draw_scrollbar(int x, int y, bool scroll_down, int count) {
    if (count <= 0)
        return;

    if (scroll_down) {
        vgi_gui_printf(x, y + GUI_FONT_H, "\\/");
        if (count > 99)
            vgi_gui_print(x, y, "..");
        else
            vgi_gui_printf(x + (count > 9 ? 0 : GUI_FONT_W / 2), y,
                            "%d", count);
    } else {
        vgi_gui_printf(x, y, "/\\");
        if (count > 99)
            vgi_gui_print(x, y + GUI_FONT_H, "..");
        else
            vgi_gui_printf(x + (count > 9 ? 0 : GUI_FONT_W / 2), y + GUI_FONT_H,
                            "%d", count);
    }
}
