#ifndef _DISPLAY_H_
#define _DISPLAY_H_

void updateFrameBuf(const SceDisplayFrameBuf *param);
void setBgColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void setTextColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void setTextScale(uint8_t scale);

uint32_t getTextWidth(const char *str);

void clearScreen();

void drawCharacter(int character, int x, int y);
void drawString(int x, int y, const char *str);
void drawStringF(int x, int y, const char *format, ...);

#endif
