/*  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DISPLAY_C
#define DISPLAY_C
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_thread.h>
#include "SDL_FontCache.h"
#include"display.h"
#include"ff_handling.h"
#include"util.h"
#include "utf8.h"

SDL_Surface* DISPLAY_winSurface = NULL;
SDL_Window* DISPLAY_window = NULL;
TTF_Font* DISPLAY_font=NULL;
FC_Font* DISPLAY_fc_font=NULL;
SDL_Renderer* DISPLAY_renderer;
//SDL_Texture *DISPLAY_text;
int DISPLAY_maxW=0,DISPLAY_maxH=0;
int DISPLAY_cW=0,DISPLAY_cH=0;
int DISPLAY_oX=0,DISPLAY_oY=0;
uint16_t DISPLAY_screenSaverState=0;
uint64_t DISPLAY_screenSaverLastTime=0;
uint8_t DISPLAY_rotateActive=0;
uint8_t noUseSDL=1;
uint8_t DISPLAY_invalidateCache=0;


uint32_t DISPLAY_grid[4][16]={0};
uint32_t DISPLAY_gridLast[4][16]={0};
char DISPLAY_fontPath[FF_FILE_PATH_MAX];


void DISPLAY_init(char *fontPath,uint8_t runsOnDesktop){

    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO);
    SDL_LogSetPriority(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO);
    SDL_LogSetPriority(SDL_LOG_CATEGORY_AUDIO,SDL_LOG_PRIORITY_INFO);
    SDL_LogSetPriority(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO);

    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER |SDL_INIT_JOYSTICK ) < 0 ) {
	    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"Could not init SDL system: %s\n",SDL_GetError());
        return;
    }
    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO,"SDL_Init finished.\n");

    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
    {
	    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"SDL_GetDesktopDisplayMode failed: %s\n",SDL_GetError());
        return;
    }

    if(runsOnDesktop){
        DISPLAY_window = SDL_CreateWindow( "TINAMP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 320, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
        if ( !DISPLAY_window ) {
	        SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"Could not get SDL window: %s\n",SDL_GetError());
            return;
        }
        SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO,"Window created OK\n");
    }else{
        // Create our window
        DISPLAY_window = SDL_CreateWindow( "TINAMP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
        // Make sure creating the window succeeded
        if ( !DISPLAY_window ) {
	        SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"Could not get SDL window: %s\n",SDL_GetError());
            return;
        }
        SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO,"Window created.\n");
        SDL_SetWindowFullscreen(DISPLAY_window, SDL_WINDOW_FULLSCREEN);
    }
    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO,"Fullscreen enabled.\n");

    DISPLAY_renderer = SDL_CreateRenderer( DISPLAY_window, -1, SDL_RENDERER_ACCELERATED );
    if ( !DISPLAY_renderer ) {
	    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"Could not get SDL renderer: %s\n",SDL_GetError());
        return;
    }
    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO,"Renderer created.\n");
    DISPLAY_winSurface = SDL_CreateRGBSurface(
            0, DISPLAY_maxW, DISPLAY_maxH, 24,
            0x00FF0000,
            0x0000FF00,
            0x000000FF,
            0xFF000000
        );
    // Make sure getting the surface succeeded
    if ( !DISPLAY_winSurface ) {
	    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"Could not get SDL surface: %s\n",SDL_GetError());
        return;
    }
    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO,"Surface created.\n");

    // Fill the window with a black rectangle
    SDL_FillRect( DISPLAY_winSurface, NULL, SDL_MapRGB( DISPLAY_winSurface->format, 0, 0, 0 ) );
    // Update the window display
    SDL_UpdateWindowSurface( DISPLAY_window );
    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO,"Surface cleaned.\n");

    if ( TTF_Init() < 0 ) {
    	SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"Could not init SDL_TTF: %s\n",SDL_GetError());
        return;
    }
    strcpy(&DISPLAY_fontPath[0],fontPath);
    strcat(&DISPLAY_fontPath[0],"assets/");
    DISPLAY_setFont(0);
    SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_INFO,"SDL init finished.\n");
    noUseSDL=0;

}

void DISPLAY_close(){
    if(DISPLAY_window){
        // Destroy the window. This will also destroy the surface
        SDL_DestroyWindow( DISPLAY_window );
        // Quit SDL
        SDL_Quit();
    }
}

void DISPLAY_clearCache(){
    DISPLAY_invalidateCache=1;
}

uint8_t DISPLAY_checkUpdateNeeded(){
    if(DISPLAY_invalidateCache){
        DISPLAY_invalidateCache=0;
        return 1;
    }
    if(memcmp(&DISPLAY_grid[0][0],&DISPLAY_gridLast[0][0],sizeof(DISPLAY_grid))!=0){
        memcpy(&DISPLAY_gridLast[0][0],&DISPLAY_grid[0][0],sizeof(DISPLAY_grid));
        return 1;
    }
    return 0;
}

void DISPLAY_update(){
    if(DISPLAY_checkUpdateNeeded()==0){
	return;
    }
    if(noUseSDL){
	printf("\033[2J");
	printf("\033[0;0H");
	for(int y=0;y<4;y++){
	    for(int x=0;x<16;x++){
		if((DISPLAY_grid[y][x]!=0)&&(DISPLAY_grid[y][x]!=' ')){
		    printf("%c",DISPLAY_grid[y][x]);
		}else{
		    printf(" ");
		}
	    }
	    printf("\n");
	}

    }else{
        SDL_RenderClear( DISPLAY_renderer );

        SDL_Texture* target;
        if(DISPLAY_rotateActive){
             target = SDL_CreateTexture(DISPLAY_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, DISPLAY_maxW, DISPLAY_maxH);
            SDL_SetRenderTarget(DISPLAY_renderer, target);
        }
        SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 0, 0, 255 );
        int maxCharSizeWidth=DISPLAY_maxW/16;
        int maxCharSizeHeigth=DISPLAY_maxH/4;
        for(int y=0;y<4;y++){
            for(int x=0;x<16;x++){
                uint32_t t[2]={0};
                t[0]=DISPLAY_grid[y][x];
                if((t[0]!=0)&&(t[0]!=' ')){
                    FC_Draw(DISPLAY_fc_font, DISPLAY_renderer, x*maxCharSizeWidth+DISPLAY_oX, y*maxCharSizeHeigth+DISPLAY_oY+(maxCharSizeHeigth-DISPLAY_cH)/2, (char*)&t[0]);
                }
            }
        }
        if(DISPLAY_rotateActive){
            SDL_SetRenderTarget(DISPLAY_renderer, NULL);
            SDL_RenderCopyEx(DISPLAY_renderer,target,
                        NULL,
                        NULL,
                        180,
                        NULL,
                        SDL_FLIP_NONE);
        }
        SDL_RenderPresent( DISPLAY_renderer );
    }
}

void DISPLAY_cls(){
    memset(&DISPLAY_grid[0][0],' ',sizeof(DISPLAY_grid));
}

void DISPLAY_printStr(uint8_t x,uint8_t y, char* string){
    int len=utf8len(string);
    for(int i=0;i<len;i++){
        if((x+i<16)&&(y<4)){
            uint32_t t[2]={0};//2 UTF-8 chars
            UTIL_utf8_char_at(string, i, (char*)&t[0]);
            DISPLAY_grid[y][x+i]=t[0];
        }
    }
}

void DISPLAY_rotate(uint8_t rotate){
    if(rotate){
        DISPLAY_rotateActive=1;
    }else{
        DISPLAY_rotateActive=0;
    }

}

int64_t DISPLAY_getMaxBrightness(){
    FILE *bl;
    int64_t b=250;//default, this also works for muos where all devices have max brightness of 255, not choosing 255 though as some devices seem to interprete 255 as display off
    char line[50];
    bl=fopen("/sys/devices/platform/backlight/backlight/backlight/max_brightness","r");
    if(bl){
        if(fgets(&line[0],sizeof(line),bl)!= NULL){
	        b=atol(&line[0]);
	    }
	    fclose(bl);
    }
    return b;
}

//brightness must be between 0 and 100 and is adjusted internally to match the devices autodetected maxBrightness
//according to maximum possible device brightness
void DISPLAY_setBrightness(uint16_t brightness){
    int64_t max=DISPLAY_getMaxBrightness();
    if(max>0){
        uint32_t effectiveBrightness=max*brightness/255;
        DISPLAY_setRawBrightness(effectiveBrightness);
    }
}

//used to set the devices brightness back to what it was before leaving the application
void DISPLAY_setRawBrightness(uint16_t brightness){
    FILE *bl;
    bl=fopen("/sys/devices/platform/backlight/backlight/backlight/brightness","r+");
    if(bl){
        fprintf(bl, "%i\n",brightness);
        fclose(bl);
    }else{//muos case
        bl=fopen("/sys/kernel/debug/dispdbg/name","r+");
        if(bl){
            fprintf(bl, "lcd0\n");
            fclose(bl);
        }else{
            return;
        }
        bl=fopen("/sys/kernel/debug/dispdbg/command","r+");
        if(bl){
            fprintf(bl, "setbl\n");
            fclose(bl);
        }else{
            return;
        }
        bl=fopen("/sys/kernel/debug/dispdbg/param","r+");
        if(bl){
            fprintf(bl, "%i\n",brightness);
            fclose(bl);
        }else{
            return;
        }
        bl=fopen("/sys/kernel/debug/dispdbg/start","r+");
        if(bl){
            fprintf(bl, "1\n");
            fclose(bl);
        }else{
            return;
        }
    }
}

//used to get the initial devices brightness settings to be able to return to it when leaving the application
uint16_t DISPLAY_getRawBrightness(){
    FILE *bl;
    uint16_t b=250;
    char line[50];
    bl=fopen("/sys/devices/platform/backlight/backlight/backlight/brightness","r");
    if(bl){
        if(fgets(&line[0],sizeof(line),bl)!= NULL){
            b=atoi(&line[0]);
        }
        fclose(bl);
    }else{//muos case
        bl=fopen("/sys/kernel/debug/dispdbg/name","r+");
        if(bl){
            fprintf(bl, "lcd0\n");
            fclose(bl);
        }else{
            return 250;
        }
        bl=fopen("/sys/kernel/debug/dispdbg/command","r+");
        if(bl){
            fprintf(bl, "getbl\n");
            fclose(bl);
        }else{
            return 250;
        }
        bl=fopen("/sys/kernel/debug/dispdbg/start","r+");
        if(bl){
            fprintf(bl, "1\n");
            fclose(bl);
        }else{
            return 250;
        }
        bl=fopen("/sys/kernel/debug/dispdbg/info","r");
        if(bl){
            if(fgets(&line[0],sizeof(line),bl)!= NULL){
                b=atoi(&line[0]);
            }
            fclose(bl);
        }
    }
    return b;
}

uint8_t DISPLAY_setFont(uint16_t fontNumber){
    char fontFile[FF_FILE_PATH_MAX+100];
    sprintf(&fontFile[0],"%sfont%i.otf",&DISPLAY_fontPath[0],fontNumber);
    //printf("Using %s\n",&fontFile[0]);
    if (access(&fontFile[0], F_OK) != 0) {
        SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"Could not find SDL TTF font: %s\n",SDL_GetError());
        return 1;
    }
    if ( DISPLAY_fc_font ) {
        FC_FreeFont(DISPLAY_fc_font);
    }
    if ( DISPLAY_font ) {
        TTF_CloseFont(DISPLAY_font);
    }
    DISPLAY_font = TTF_OpenFont(fontFile, 100);
    if ( !DISPLAY_font ) {
        SDL_LogMessage(SDL_LOG_CATEGORY_VIDEO,SDL_LOG_PRIORITY_ERROR,"Could not open SDL TTF font: %s\n",SDL_GetError());
        return 1;
    }
    int w100,h100;//contain the real pixel size for the font opened in point size 100
    TTF_SizeUTF8(DISPLAY_font, "M", &w100, &h100);

    SDL_GetWindowSize(DISPLAY_window, &DISPLAY_maxW, &DISPLAY_maxH);
    //calculate best fitting charsize for 16x4 char display
    int maxCharSizeWidth=DISPLAY_maxW/16;
    int maxCharSizeHeigth=DISPLAY_maxH/4;
    //calculate offsets to position 16x4 screen in middle of display
    DISPLAY_oX=(DISPLAY_maxW-(maxCharSizeWidth*16))/2;
    DISPLAY_oY=(DISPLAY_maxH-(maxCharSizeHeigth*4))/2;

    //verify if using width as base also works for heigth, otherwise use heigth as base
    int adjustedPointSize=(maxCharSizeWidth*h100)/w100;
    if((adjustedPointSize)*4>DISPLAY_maxH){
        adjustedPointSize=maxCharSizeHeigth;
    }
#if SDL_VERSION_ATLEAST(2, 0, 18)
    TTF_SetFontSize(DISPLAY_font, (adjustedPointSize*100)/h100);
#else
    // Fallback or older-version-safe code
    TTF_CloseFont(DISPLAY_font);
    DISPLAY_font = TTF_OpenFont(fontFile, (adjustedPointSize*100)/h100);
#endif
    //FontCache stuff
    DISPLAY_fc_font = FC_CreateFont();
    FC_LoadFont(DISPLAY_fc_font, DISPLAY_renderer, fontFile, (adjustedPointSize*100)/h100, FC_MakeColor(44,255,5,255), TTF_STYLE_NORMAL);

    TTF_SizeUTF8(DISPLAY_font, "M", &w100, &h100);
    DISPLAY_cW=w100;
    DISPLAY_cH=h100;
    return 0;
}

void DISPLAY_screenSaver(){
    switch(DISPLAY_screenSaverState){
        case 0: DISPLAY_cls();DISPLAY_update();//make sure the internal grid buffer is also reset
                DISPLAY_screenSaverState++;
                DISPLAY_screenSaverLastTime=UTIL_get_time_us();
                SDL_RenderClear( DISPLAY_renderer );
                SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 0, 0, 255 );
                SDL_RenderPresent( DISPLAY_renderer );
                break;
        case 1: if(UTIL_get_time_us()-DISPLAY_screenSaverLastTime>1000000){
                    DISPLAY_screenSaverState++;
                    DISPLAY_screenSaverLastTime=UTIL_get_time_us();
                    SDL_RenderClear( DISPLAY_renderer );
                    SDL_SetRenderDrawColor( DISPLAY_renderer, 255, 255,255, 255 );
                    SDL_RenderPresent( DISPLAY_renderer );
                }
                break;
        case 2: if(UTIL_get_time_us()-DISPLAY_screenSaverLastTime>1000000){
                    DISPLAY_screenSaverState++;
                    DISPLAY_screenSaverLastTime=UTIL_get_time_us();
                    SDL_RenderClear( DISPLAY_renderer );
                    SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 0, 0, 255 );
                    SDL_RenderPresent( DISPLAY_renderer );
                }
                break;
        case 3: if(UTIL_get_time_us()-DISPLAY_screenSaverLastTime>1000000){
                    DISPLAY_screenSaverState++;
                    DISPLAY_screenSaverLastTime=UTIL_get_time_us();
                    SDL_RenderClear( DISPLAY_renderer );
                    SDL_SetRenderDrawColor( DISPLAY_renderer, 255, 0, 0, 255 );
                    SDL_RenderPresent( DISPLAY_renderer );
                }
                break;
        case 4: if(UTIL_get_time_us()-DISPLAY_screenSaverLastTime>1000000){
                    DISPLAY_screenSaverState++;
                    DISPLAY_screenSaverLastTime=UTIL_get_time_us();
                    SDL_RenderClear( DISPLAY_renderer );
                    SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 0, 0, 255 );
                    SDL_RenderPresent( DISPLAY_renderer );
                }
                break;
        case 5: if(UTIL_get_time_us()-DISPLAY_screenSaverLastTime>1000000){
                    DISPLAY_screenSaverState++;
                    DISPLAY_screenSaverLastTime=UTIL_get_time_us();
                    SDL_RenderClear( DISPLAY_renderer );
                    SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 255, 0, 255 );
                    SDL_RenderPresent( DISPLAY_renderer );
                }
                break;
        case 6: if(UTIL_get_time_us()-DISPLAY_screenSaverLastTime>1000000){
                    DISPLAY_screenSaverState++;
                    DISPLAY_screenSaverLastTime=UTIL_get_time_us();
                    SDL_RenderClear( DISPLAY_renderer );
                    SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 0, 0, 255 );
                    SDL_RenderPresent( DISPLAY_renderer );
                }
                break;
        case 7: if(UTIL_get_time_us()-DISPLAY_screenSaverLastTime>1000000){
                    DISPLAY_screenSaverState++;
                    DISPLAY_screenSaverLastTime=UTIL_get_time_us();
                    SDL_RenderClear( DISPLAY_renderer );
                    SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 0, 255, 255 );
                    SDL_RenderPresent( DISPLAY_renderer );
                }
                break;
        case 8: if(UTIL_get_time_us()-DISPLAY_screenSaverLastTime>1000000){
                    DISPLAY_screenSaverState=0;
                }
                break;

        default:DISPLAY_screenSaverState=0;
                break;
    }
}

void DISPLAY_screenSaverOff(){
    DISPLAY_screenSaverState=0;
    SDL_RenderClear( DISPLAY_renderer );
    SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 0, 0, 255 );
    SDL_RenderPresent( DISPLAY_renderer );
    SDL_Delay(10);
    SDL_RenderClear( DISPLAY_renderer );
    SDL_SetRenderDrawColor( DISPLAY_renderer, 0, 0, 0, 255 );
    SDL_RenderPresent( DISPLAY_renderer );
    SDL_Delay(100);
}

#endif
