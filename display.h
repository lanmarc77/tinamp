#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdint.h>


void DISPLAY_init(char *fontPath,uint8_t runsOnDesktop);
void DISPLAY_close();
void DISPLAY_update();
void DISPLAY_cls();
void DISPLAY_printStr(uint8_t x,uint8_t y, char* string);
void DISPLAY_rotate(uint8_t rotate);
void DISPLAY_setBrightness(uint16_t brightness);
uint16_t DISPLAY_getRawBrightness();
void DISPLAY_setRawBrightness(uint16_t brightness);
uint8_t DISPLAY_setFont(uint16_t fontNumber);
void DISPLAY_screenSaver();
void DISPLAY_screenSaverOff();
void DISPLAY_clearCache();
#endif



