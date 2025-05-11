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
#ifndef UI_ELEMENTS_C
#define UI_ELEMENTS_C

#include "ui_elements.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "display.h"
#include "util.h"
#include "battery.h"
#include "utf8.h"

bool UI_ELEMENTS_displayDark=false;

uint8_t UI_ELEMENTS_disablePosition=16*4;

/**
  * @brief simply prints a string to the given screen position
  *
  * @param x x position 
  * @param y y position
  * @param string pointer to a string
  * 
  */
void UI_ELEMENTS_printStr(uint8_t x,uint8_t y, char* string){
    DISPLAY_printStr(x,y,string);
}

/**
  * @brief disables showing charactes starting with a specific position on the screen
  *
  * @param startPos 0 (all is displayed)...16*4 (dnothing is displayed)
  * 
  */
void UI_ELEMENTS_disableChars(uint8_t startPos){
    if(startPos>16*4) startPos=16*4;
    UI_ELEMENTS_disablePosition=(16*4)-startPos;
}

/**
  * @brief shows the progress meter
  *
  * @param percent percent 0...100
  * 
  */
 void UI_ELEMENTS_progressBar(uint8_t percent){
    char progress[17];
    uint8_t i;
    if(percent>100) percent=100;
    uint8_t numCharsBig=(14*percent)/100;
    uint8_t numCharsRemainder=(14*percent)%100;

    memset(&progress[0],' ',sizeof(progress));
    progress[0]='[';progress[15]=']';if(percent<50) progress[8]='.';progress[16]=0;
    for(i=0;i<numCharsBig;i++){
        progress[i+1]='=';
    }
    if(numCharsRemainder>=66){
        progress[i+1]='-';
    }else if(numCharsRemainder>=33){
        progress[i+1]='_';
    }
    DISPLAY_printStr(0,3,&progress[0]);
}

int32_t UI_ELEMENTS_lastSecondsDisplayed=-1;
/**
  * @brief shows a time in hours minutes seconds
  *
  * @param playedS time to display in seconds
  * @param displayMode 0=normal,1=hours+minutes blink,2=playedS is displayed as negative number
  * 
  */
void UI_ELEMENTS_timePlayed(int32_t playedS,uint8_t displayMode){
    char timeString[17];
    if((UI_ELEMENTS_lastSecondsDisplayed>=0)||(UI_ELEMENTS_lastSecondsDisplayed!=playedS)){
        UI_ELEMENTS_lastSecondsDisplayed=playedS;
        uint16_t hours=playedS/3600;
        uint8_t minutes=(playedS%3600)/60;
        uint8_t seconds=(playedS%3600)%60;
        uint64_t now=UTIL_get_time_us();
        if((displayMode==1)&&(((now/1000)%1000)<100)){
            if(hours==0){
                sprintf(&timeString[0],"       :%02d",seconds);
            }else{
                sprintf(&timeString[0],"    :  :%02d",seconds);
            }
        }else{
            if(displayMode==2){
                if(hours==0){
                    sprintf(&timeString[0],"    -%02d:%02d",minutes,seconds);
                }else{
                    sprintf(&timeString[0]," %3d:%02d:%02d",-hours,minutes,seconds);
                }
            }else{
                if(hours==0){
                    sprintf(&timeString[0],"     %02d:%02d",minutes,seconds);
                }else{
                    sprintf(&timeString[0],"%4d:%02d:%02d",hours,minutes,seconds);
                }
            }
        }
        DISPLAY_printStr(0,2,&timeString[0]);
    }
}

/**
  * @brief displays a battery level
  *
  */
void UI_ELEMENTS_batteryIndicator(){
    char b[12];
    uint8_t percent=0;
    percent=BATTERY_getPercent();
    if(percent>=100){
        sprintf(b,"%1d",percent/100);
        DISPLAY_printStr(15,0,b);
    }
    if(percent>=10){
        sprintf(b,"%1d",(percent%100)/10);
        DISPLAY_printStr(15,1,b);
    }
    if(percent==0){
        sprintf(b," ");
    }else{
        sprintf(b,"%1d",(percent%100)%10);
    }
    DISPLAY_printStr(15,2,b);
}

/**
  * @brief displays different type of huge symbols
  *
  * @param symbol symbol number to display
  * 
  */
void UI_ELEMENTS_mainSymbol(uint8_t symbol){
    uint64_t now=UTIL_get_time_us();
    char b[17];
    b[0]=0;
    if(symbol==1){//play symbol
        DISPLAY_printStr(11,2,"|> ");
    }else if(symbol==2){//pause symbol
        DISPLAY_printStr(11,2,"|| ");
    }else if(symbol==3){//no SD card symbol
        DISPLAY_printStr(0,0,"    -------     ");
        DISPLAY_printStr(0,1,"   | SD ?  |    ");
        DISPLAY_printStr(0,2,"   |______/     ");
    }else if(symbol==4){//all folder scan symbol
        DISPLAY_printStr(0,0,"     |---|      ");
        DISPLAY_printStr(0,1,"     | ? |      ");
        DISPLAY_printStr(0,2,"     |---|      ");
    }else if(symbol==5){//no folder found symbol
        DISPLAY_printStr(0,0,"     |---|      ");
        DISPLAY_printStr(0,1,"     | 0 |      ");
        DISPLAY_printStr(0,2,"     |---|      ");
    }else if(symbol==6){//folder selection
        DISPLAY_printStr(0,1," |------------| ");
        DISPLAY_printStr(0,2," |            | ");
        DISPLAY_printStr(0,3," |------------| ");
    }else if(symbol==7){//single folder scan symbol
        if(((now/1000)%1000)<500){
            DISPLAY_printStr(0,0," |------------| ");
            DISPLAY_printStr(0,1," |  B <==> A  | ");
            DISPLAY_printStr(0,2," |------------| ");
        }else{
            DISPLAY_printStr(0,0," |------------| ");
            DISPLAY_printStr(0,1," |  A <==> B  | ");
            DISPLAY_printStr(0,2," |------------| ");
        }
    }else if(symbol==8){//switch off
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"    (-_-)Zzz    ");
        DISPLAY_printStr(0,2,"                ");
        DISPLAY_printStr(0,3,"                ");
    }else if(symbol==9){//starting
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"     (o_o)      ");
        DISPLAY_printStr(0,2,"                ");
        sprintf(&b[0],"   v%02X.%02X.%02X    ",FW_MAJOR,FW_MINOR,FW_PATCH);
        DISPLAY_printStr(0,3,&b[0]);
    }else if(symbol==10){//erasing countdown
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"     (o o)      ");
        DISPLAY_printStr(0,2,"       O        ");
    }else if(symbol==11){//erasing
        DISPLAY_printStr(0,0,"0000000000000000");
        DISPLAY_printStr(0,1,"0000000000000000");
        DISPLAY_printStr(0,2,"0000000000000000");
        DISPLAY_printStr(0,3,"0000000000000000");
    }else if(symbol==12){//battery low
        DISPLAY_printStr(0,0,"      _--_      ");
        DISPLAY_printStr(0,1,"     |    |     ");
        DISPLAY_printStr(0,2,"     |    |     ");
        DISPLAY_printStr(0,3,"     |####|     ");
    }else if(symbol==13){//pause symbol with repeat folder option on
        DISPLAY_printStr(11,2,"||*");
    }else if(symbol==14){//fw upgrade init
        sprintf(&b[0]," ||    %02X.%02X.%02X ",FW_MAJOR,FW_MINOR,FW_PATCH);
        DISPLAY_printStr(0,0,&b[0]);
        DISPLAY_printStr(0,1,"-  -     vvv    ");
        DISPLAY_printStr(0,2,"-  -      ?     ");
        DISPLAY_printStr(0,3," ||    XX.XX.XX ");
    }else if(symbol==15){//fw upgrade running
        DISPLAY_printStr(0,0,"       ||       ");
        DISPLAY_printStr(0,1,"    -      -    ");
        DISPLAY_printStr(0,2,"    -      -    ");
        DISPLAY_printStr(0,3,"       ||       ");
    }else if(symbol==16){//wakeup timer
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"(-_-)  ->  (o_o)");
        DISPLAY_printStr(0,2,"                ");
        DISPLAY_printStr(0,3,"                ");
    }else if(symbol==17){//screen rotation
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"   -------->    ");
        DISPLAY_printStr(0,2,"  |   ???   |   ");
        DISPLAY_printStr(0,3,"   <--------    ");
    }else if(symbol==18){//rotary encoder direction
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1," o o            ");
        DISPLAY_printStr(0,2,"o   o           ");
        DISPLAY_printStr(0,3," o o            ");
    }else if(symbol==19){//rotary encoder speed
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1," o o            ");
        DISPLAY_printStr(0,2,"o   o           ");
        DISPLAY_printStr(0,3," o o            ");
    }else if(symbol==20){//bookmark deletion
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"   Bookmarks    ");
        DISPLAY_printStr(0,2,"?????? -> 0     ");
        DISPLAY_printStr(0,3,"       ?        ");
    }else if(symbol==21){//brightness setup
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"\\\\\\\\\\\\   //////");
        DISPLAY_printStr(0,2,"      xxx       ");
        DISPLAY_printStr(0,3,"//////   \\\\\\\\\\\\");
    }else if(symbol==22){//setup selection
        DISPLAY_printStr(0,0,"     *******    ");
        DISPLAY_printStr(0,1,"   /  Setup  \\  ");
        DISPLAY_printStr(0,2,"   \\         /  ");
        DISPLAY_printStr(0,3,"     *******    ");
    }else if(symbol==23){//setup image persisted
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"    firmware    ");
        DISPLAY_printStr(0,2,"    persisted   ");
        DISPLAY_printStr(0,3,"       OK       ");
    }else if(symbol==24){//setup image in TEST mode
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"     persist    ");
        DISPLAY_printStr(0,2,"     firmware   ");
        DISPLAY_printStr(0,3,"        ?       ");
    }else if(symbol==25){//setup full functions
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"       all      ");
        DISPLAY_printStr(0,2,"    functions   ");
        DISPLAY_printStr(0,3,"                ");
    }else if(symbol==26){//setup reduced functions
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"     reduced    ");
        DISPLAY_printStr(0,2,"     functions  ");
        DISPLAY_printStr(0,3,"                ");
    }else if(symbol==27){//setup font
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"ABCDEfghij012345");
        DISPLAY_printStr(0,2,"                ");
        DISPLAY_printStr(0,3,"/=_-*.!#?[<|>]+\\");
    }else if(symbol==28){//setup a-b normal
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"        --> (A) ");
        DISPLAY_printStr(0,2,"        |       ");
        DISPLAY_printStr(0,3," (B) <---       ");
    }else if(symbol==29){//setup a-b switched
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"        --> (B) ");
        DISPLAY_printStr(0,2,"        |       ");
        DISPLAY_printStr(0,3," (A) <---       ");
    }else if(symbol==30){//setup screen saver oled
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"                ");
        DISPLAY_printStr(0,2,"    (^_^)Zzz    ");
        DISPLAY_printStr(0,3,"      OLED      ");
    }else if(symbol==31){//setup screen saver lcd
        DISPLAY_printStr(0,0,"                ");
        DISPLAY_printStr(0,1,"                ");
        DISPLAY_printStr(0,2,"    (^_^)Zzz    ");
        DISPLAY_printStr(0,3,"      LCD       ");
    }
}

/**
  * @brief allows to display a number selection element
  * 
  * @param x position on screen
  * @param y position on screen
  * @param selectedNumber the currently selected number
  * @param amountOfNumbers highest number to select
  * @param blinkMode 0=number is displayed solid, 1=number is blinking
  *
  */
void UI_ELEMENTS_numberSelect(uint8_t x,uint8_t y,uint16_t selectedNumber,uint16_t amountOfNumbers,uint8_t blinkMode){
    char chapterString[17];
    char chapterCompleteString[17];
    uint64_t now=UTIL_get_time_us();
    if(blinkMode==1){
        if(((now/1000)%1000)<100){
            sprintf(&chapterString[0],"/%d",amountOfNumbers);
        }else{
            sprintf(&chapterString[0],"%d/%d",selectedNumber,amountOfNumbers);
        }
    }else{
        sprintf(&chapterString[0],"%d/%d",selectedNumber,amountOfNumbers);
    }
    sprintf(&chapterCompleteString[0],"%10s",&chapterString[0]);
    DISPLAY_printStr(x,y,&chapterCompleteString[0]);
}

/**
  * @brief allows to display a time element in hours and minutes
  * 
  * @param x position on screen
  * @param y position on screen
  * @param secondsTime the currently selected time
  * @param blinkMode 0=time is displayed solid, 1=time is blinking
  *
  */
void UI_ELEMENTS_timeSelect(uint8_t x,uint8_t y,uint64_t secondsTime,uint8_t blinkMode){
    char timeString[17];
    uint64_t now=UTIL_get_time_us();
    if(blinkMode==1){
        if(((now/1000)%1000)<100){
            sprintf(&timeString[0],"  :  ");
        }else{
            sprintf(&timeString[0],"%02i:%02i",(uint16_t)(secondsTime/3600),(uint16_t)((secondsTime%3600)/60));
        }
    }else{
        sprintf(&timeString[0],"%02i:%02i",(uint16_t)(secondsTime/3600),(uint16_t)((secondsTime%3600)/60));
    }
    DISPLAY_printStr(x,y,&timeString[0]);
}

/**
  * @brief shows equalizer preset
  * 
  * @param equalizer preset, 0=netural
  *
  */
void UI_ELEMENTS_equalizer(uint8_t equalizer){
    switch(equalizer){
        case 0: 
                DISPLAY_printStr(5,1,"normal");
                break;
        case 1: 
                DISPLAY_printStr(6,1,"bass");
                break;
        case 2: 
                DISPLAY_printStr(1,1,"bass and treble");
                break;
        case 3: 
                DISPLAY_printStr(5,1,"treble");
                break;
        case 4: 
                DISPLAY_printStr(3,1,"headphone");
                break;
        case 5:
                DISPLAY_printStr(5,1,"voice 1");
                break;
        case 6: 
                DISPLAY_printStr(5,1,"voice 2");
                break;
    }
}

/**
  * @brief shows repeat mode
  * 
  * @param repeatMode 0=no repeat, bit 1=folder repeat enabled, bit 2=autostart enabled
  *
  */
void UI_ELEMENTS_repeatMode(uint8_t repeatMode){
    if(repeatMode&UI_MAIN_REPEAT_FOLDER){
        DISPLAY_printStr(3,1,"|>  <->  ||");
    }else{
        DISPLAY_printStr(3,1,"|>  -->  ||");
    }
}

/**
  * @brief shows play speed
  * 
  * @param playSpeed 50...300 meaning x0.5...x3.0
  *
  */
void UI_ELEMENTS_playSpeed(uint16_t playSpeed){
    char number[10];
    DISPLAY_printStr(5,1,">>>>>>>>");
    sprintf(&number[0]," x%i.%02i ",playSpeed/100,playSpeed%100);
    DISPLAY_printStr(5,2,&number[0]);
}

/**
  * @brief shows a volume meter full screen
  * 
  * @param volume volume level to show 0...10000
  *
  */
void UI_ELEMENTS_volume(int64_t volume){
    char screen[16*4];
    char line[17];
    char number[10];
    uint8_t i=0;
    uint8_t jump=0;
    line[16]=0;
    for(i=0;i<sizeof(screen);i++){
        if(((16*4*volume)/150)>i){
            if(i>=(16*4*100)/150){
                screen[i]='+';
            }else{
                screen[i]='#';
            }
            if(i>=(16*2)) jump=1;
        }else{
            screen[i]=' ';
        }
    }
    sprintf(&number[0]," %3lu ",volume);
    if(jump){
        memcpy(&screen[21],&number[0],5);
    }else{
        memcpy(&screen[37],&number[0],5);
    }

    for(i=0;i<4;i++){
        memcpy(&line[0],&screen[16*i],16);
        DISPLAY_printStr(0,i,&line[0]);
    }
}

uint16_t UI_ELEMENTS_lastTextScrollyPos=0;
uint64_t UI_ELEMENTS_lastTextScrollyTimestamp=0;
/**
  * @brief resets the textScrolly state machine so that the next call
  *        is handled as it would be the first (beginning of text visible)
  * 
  * @param x position on screen
  * @param y position on screen
  * @param length the reserved maximum available length in chars on the display to scroll within
  *
  */
void UI_ELEMENTS_textScrollyReset(uint8_t x,uint8_t y, uint8_t length){
    char scrollyText[17];
    uint8_t i;
    for(i=0;i<length;i++){
        scrollyText[i]=' ';
    }
    scrollyText[i]=0;
    UI_ELEMENTS_lastTextScrollyTimestamp=0;
    UI_ELEMENTS_lastTextScrollyPos=0;
    DISPLAY_printStr(x,y,&scrollyText[0]);
}

/**
  * @brief implements a general text scrolly that only scrolls if needed
  *        and can therefore also be used as simple text output
  *
  * @param x position on screen
  * @param y position on screen
  * @param length the reserved maximum available length in chars on the display to scroll within
  * @param string pointer to the text to display
  * 
  */
void UI_ELEMENTS_textScrolly(uint8_t x,uint8_t y, uint8_t length, char *string){
    uint32_t scrollyText[16+1];//UTF8=4 byte per char +1 tailing 0
    uint8_t sl=utf8len(string);
    memset(&scrollyText[0],0,sizeof(scrollyText));
    if(UI_ELEMENTS_lastTextScrollyTimestamp!=0){
        if(UI_ELEMENTS_lastTextScrollyPos==0){//full display length before scrolling starts is longer
            if((UTIL_get_time_us()-UI_ELEMENTS_lastTextScrollyTimestamp)/1000>100*length){
                UI_ELEMENTS_lastTextScrollyPos++;
                UI_ELEMENTS_lastTextScrollyTimestamp=UTIL_get_time_us();
            }
        }else if((UTIL_get_time_us()-UI_ELEMENTS_lastTextScrollyTimestamp)/1000>125){
            UI_ELEMENTS_lastTextScrollyPos++;
            UI_ELEMENTS_lastTextScrollyTimestamp=UTIL_get_time_us();
        }
    }else{
        UI_ELEMENTS_lastTextScrollyTimestamp=UTIL_get_time_us();
    }
    if(UI_ELEMENTS_lastTextScrollyPos>=sl) UI_ELEMENTS_lastTextScrollyPos=0;
    if(length>=sl){//no need to scroll if the string fits into the assigned space completely
        UI_ELEMENTS_lastTextScrollyPos=0;
    }
    for(uint8_t i=0;i<length;i++){
        if(UI_ELEMENTS_lastTextScrollyPos+i>=sl){
            char t[2]={0};t[0]=' ';
            utf8cat((char*)&scrollyText[0],&t[0]);
        }else{
            uint32_t t[2]={0};
            UTIL_utf8_char_at(string, UI_ELEMENTS_lastTextScrollyPos+i, (char*)&t[0]);
            utf8cat((char*)&scrollyText[0],(char*)&t[0]);
        }
    }
    DISPLAY_printStr(x,y,(char*)&scrollyText[0]);
}

/**
  * @brief displays a countdown time in hours or minutes or seconds as two digit number
  *        in 3 char wide frame
  *
  * @param secondsLeft countdown in seconds
  * 
  */
void UI_ELEMENTS_sleepTimeLeft(uint32_t secondsLeft){
    char timeString[12];
    if(secondsLeft==0){
        return;
    }
    if(secondsLeft>3599){
        sprintf(&timeString[0],"%02uh",(secondsLeft+29)/3600);
    }else if(secondsLeft>99){
        sprintf(&timeString[0],"%02um",(secondsLeft+29)/60);
    }else{
        sprintf(&timeString[0],"%02us",secondsLeft);

    }
    DISPLAY_printStr(11,1,&timeString[0]);

}

/**
  * @brief updates the screen from the internal buffer
  *        clears the screen if the display should be off
  *
  */
void UI_ELEMENTS_update(){
    if(UI_ELEMENTS_displayDark==false){
        DISPLAY_update();
    }else{
        DISPLAY_cls();
    } 
}

/**
  * @brief clears the screen, works instantly
  *
  */
void UI_ELEMENTS_cls(){
    DISPLAY_cls();
}

/**
  * @brief switches the display on by internally enabling the update function
  *
  */
void UI_ELEMENTS_displayOn(){
    UI_ELEMENTS_displayDark=false;
    UI_ELEMENTS_disableChars(0);
}

/**
  * @brief checks if the display is off by internally disabled update function
  *
  * @return true=display is off, false=display is on and being updated
  */
bool UI_ELEMENTS_isDisplayOff(){
    return UI_ELEMENTS_displayDark;
}

/**
  * @brief switches the display off by internally disabling the update function
  *        also clears the screen instantly
  *
  */
void UI_ELEMENTS_displayOff(){
    DISPLAY_cls();
    UI_ELEMENTS_update();//actually also display the empty display
    UI_ELEMENTS_displayDark=true;
}

/**
  * @brief initializes the physical display and clears it
  *
  */
void UI_ELEMENTS_init(){
    DISPLAY_cls();
}

/**
  * @brief rotates the screen by 180°
  * 
  * @param rotate 0=normal,1=rotate
  *
  */
void UI_ELEMENTS_rotate(uint8_t rotate){
    DISPLAY_rotate(rotate);
}


/**
  * @brief sets the brightness of the used display
  * 
  * @param rotate 0=off/minimum ...255=maximum
  *
  */
void UI_ELEMENTS_setBrightness(uint8_t brightness){
    DISPLAY_setBrightness(brightness);
}

#endif
