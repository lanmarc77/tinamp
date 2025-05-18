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
#ifndef TINAMP_C
#define TINAMP_C
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <vlc/vlc.h>
#include <SDL2/SDL_log.h>
#include "ui_elements.h"
#include "screens.h"
#include "SDL_FontCache.h"
#include "util.h"
#include "display.h"
#include "input.h"
#include "queue.h"
#include "sd_play.h"
#include "saves.h"
#include "format_helper.h"
#include "ff_handling.h"
#include "utf8.h"


uint8_t UI_MAIN_setupMenu();

uint8_t quit = 0;
int fontNumber=0;

Queue fromKeyQueue, fromVlcQueue, toFFScanner, fromFFScanner;
QueueSet fromKeyAndVlcQueue;
uint8_t reducedMode=0;
SAVES_settings_t UI_MAIN_settings;

#define UI_MAIN_RUN_SM_INIT 0
#define UI_MAIN_RUN_SM_FOLDER_SCAN 5
#define UI_MAIN_RUN_SM_FOLDER_SELECTION 10
#define UI_MAIN_RUN_SM_SETUP_MENU 13
#define UI_MAIN_RUN_SM_BOOK_SCAN 15
#define UI_MAIN_RUN_SM_PAUSED 20
#define UI_MAIN_RUN_SM_PLAY_INIT 24
#define UI_MAIN_RUN_SM_PLAYING 25
#define UI_MAIN_DEFAULT_SLEEP_TIME_S 300

uint32_t UI_MAIN_timeDiffNowS(uint64_t timeStamp){
    uint64_t now=UTIL_get_time_us();
    if(timeStamp>now){
        return (timeStamp-now)/1000000;
    }
    return 0;
}

char UI_MAIN_baseFolder[FF_FILE_PATH_MAX]={0};
char* UI_MAIN_searchString=NULL;
int32_t* UI_MAIN_searchId=NULL;

uint16_t UI_MAIN_sortedBookIDs[FF_MAX_FOLDER_ELEMENTS];
uint16_t UI_MAIN_amountOfBooks=0;
int UI_MAIN_scanSortTaskAllBooks(void *user_data){
    memset(&UI_MAIN_sortedBookIDs[0],0,sizeof(UI_MAIN_sortedBookIDs));
    FF_getList(&UI_MAIN_baseFolder[0],&UI_MAIN_amountOfBooks,&UI_MAIN_sortedBookIDs[0],0,&fromFFScanner,&toFFScanner,UI_MAIN_searchString,UI_MAIN_searchId);
    return 0;
}

char UI_MAIN_folderPath[FF_FILE_PATH_MAX];
uint16_t UI_MAIN_amountOfFiles=0;
uint16_t UI_MAIN_sortedFileIDs[FF_MAX_SORT_ELEMENTS];
int UI_MAIN_scanSortTaskOneBook(void *user_data){
    memset(&UI_MAIN_sortedFileIDs[0],0,sizeof(UI_MAIN_sortedFileIDs));
    FF_getList(&UI_MAIN_folderPath[0],&UI_MAIN_amountOfFiles,&UI_MAIN_sortedFileIDs[0],1,&fromFFScanner,&toFFScanner,UI_MAIN_searchString,UI_MAIN_searchId);
    return 0;
}

uint64_t UI_MAIN_wakeupTimer=0;
void UI_MAIN_setWakeupTimer(uint64_t timer){
    UI_MAIN_wakeupTimer=timer;
}

uint64_t UI_MAIN_getWakeupTimer(){
    return UI_MAIN_wakeupTimer;
}

uint64_t UI_MAIN_offTimestamp=0;
uint64_t UI_MAIN_lastKeyPressedTime=0;
int UI_MAIN_originalBacklight=250;
/*
 * a function for everywhere needed reading of the main message queue
 * also handles timing for screen saver
 * readFlag: bit0=key, bit1=vlc
 *
 * returns bit0=set key input, bit1 set vlc input
*/
uint8_t UI_MAIN_readKeyAndVLCQueue(uint8_t readFlag,uint16_t waitTime,QueueData *rxData){
    Queue *ready=NULL;
    uint64_t now=UTIL_get_time_us();
    if((ready=queueset_wait_timeout(&fromKeyAndVlcQueue, waitTime)) != NULL ){//wait for incoming key messages
        rxData->type=QUEUE_DATA_NONE;//avoid leftovers from old memory cells
        if(ready==&fromKeyQueue){
            queue_pop(&fromKeyQueue,rxData);
            if(rxData->type==QUEUE_DATA_INPUT){
                if(rxData->input_message.key==INPUT_KEY_OTHER) return 0;//ignore all other unknown keys
                UI_MAIN_lastKeyPressedTime=now;
                UI_MAIN_offTimestamp=now;
                if(UI_ELEMENTS_isDisplayOff()){
                    UI_ELEMENTS_displayOn();DISPLAY_screenSaverOff();UI_ELEMENTS_setBrightness(UI_MAIN_settings.brightness);
                    if(readFlag&1){
                        return 0;//throw away the key because screen was only switched on
                    }
                }
                if(rxData->input_message.key==INPUT_WINDOW_RESIZE){
                    DISPLAY_setFont(UI_MAIN_settings.fontNumber);
                    DISPLAY_clearCache();
                }
                if(readFlag&1){
                    return 1;//emit received key data
                }
            }
        }
        if(ready==&fromVlcQueue) {
            queue_pop(&fromVlcQueue,rxData);
            UI_MAIN_offTimestamp=now;
            if(rxData->type==QUEUE_DATA_SD_PLAY){
                if(readFlag&2){
                    return 2;//emit received vlc data
                }
            }
        }
    }
    return 0;//nothing received
}

uint8_t UI_MAIN_directoryExists(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

int main(int argc, char* argv[])
{
    UI_MAIN_lastKeyPressedTime=UTIL_get_time_us();
    UI_MAIN_offTimestamp=UTIL_get_time_us();
    uint8_t runsOnDesktop=0;

    char localPath[FF_FILE_PATH_MAX]={0};
    if (argc > 0 && argv[0]) {
        char *slash = strrchr(argv[0], '/');  // Find last '/'
        uint16_t len = slash - argv[0] +1;//+1 to include the trailing slash
        strncpy(localPath, argv[0], len);
        localPath[len] = '\0';
        if(strcmp(&argv[0][strlen(argv[0])-5],"amd64")==0) runsOnDesktop=1;
    }

    strcpy(&UI_MAIN_baseFolder[0],&localPath[0]);
    strcat(&UI_MAIN_baseFolder[0],"../../audiobooks");
    if(!UI_MAIN_directoryExists(&UI_MAIN_baseFolder[0])){//audiobooks two directories above?
        strcpy(&UI_MAIN_baseFolder[0],&localPath[0]);
        strcat(&UI_MAIN_baseFolder[0],"../../ROMS/audiobooks");
        if(!UI_MAIN_directoryExists(&UI_MAIN_baseFolder[0])){//audiobooks two directories above within ROMS folder for muos?
            strcpy(&UI_MAIN_baseFolder[0],&localPath[0]);
            strcat(&UI_MAIN_baseFolder[0],"audiobooks");
            if(!UI_MAIN_directoryExists(&UI_MAIN_baseFolder[0])){//audiobooks within the ports folder?
                //if we end up here the following scan will fail resulting in the application to quit
            }
        }
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    queue_init(&fromKeyQueue);
    queue_init(&fromVlcQueue);
    queue_init(&toFFScanner);
    queue_init(&fromFFScanner);
    Queue* all[] = { &fromKeyQueue, &fromVlcQueue };
    queueset_init(&fromKeyAndVlcQueue, all, 2);

    DISPLAY_init(&localPath[0],runsOnDesktop);
    UI_MAIN_originalBacklight=DISPLAY_getRawBrightness();
    DISPLAY_cls();
    DISPLAY_update();
    SCREENS_startUp(4);
    INPUT_init(&fromKeyQueue);
    SAVES_init(&localPath[0]);

    SD_PLAY_init(&fromVlcQueue);
    int64_t UI_MAIN_volume=100;
    SD_PLAY_setVolume(UI_MAIN_volume);

    SDL_version ver;
    SDL_VERSION(&ver);
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"Compiled against SDL version: %02u.%02u.%02u\n",ver.major,ver.minor,ver.patch);
    SDL_GetVersion(&ver);
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"Detected SDL version: %02u.%02u.%02u\n",ver.major,ver.minor,ver.patch);
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"LibVLC version: %s\n",libvlc_get_version());
    
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"Using base folder: %s\n",&UI_MAIN_baseFolder[0]);

    char governor[250]={0};
    char governorP[250]={0};
    UTIL_getCPUGovernor(&governor[0]);
    if(governor[0]!=0){
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"Detected CPU governor: %s. Trying to set powersave.\n",&governor[0]);
        UTIL_setCPUGovernor("powersave");
        UTIL_getCPUGovernor(&governorP[0]);
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"New CPU governor: %s.\n",&governorP[0]);
    }
    

    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"Entering main event loop.\n");

    uint8_t mainSM=UI_MAIN_RUN_SM_INIT;
    uint8_t pauseMode=0;
    uint16_t selectedFile=1;
    uint16_t selectedFolder=1;
    uint16_t oldSelectedFile=1;
    uint16_t currentPlayMinute=0;
    uint8_t currentPlaySecond=0;
    uint16_t allPlayMinute=0;
    uint8_t allPlaySecond=0;
    uint16_t newPlayMinute=0;
    uint16_t currentPlaySpeed=100; //100=1.00, 300=3.00, 50=0.5
    uint8_t currentEqualizer=0;
    uint8_t currentRepeatMode=0;
    uint8_t currentFinishedFlag=0;
    uint8_t playOverlayMode=0;
    uint8_t saveFlag=0;
    uint8_t percent=0;
    uint64_t now;
    uint64_t lastAutoSave;
    uint64_t sleepTimeOffTime=0;//the timestamp when the currently setup sleeptime is reached
    uint64_t reducedModeLastTimestamp;
    QueueData rxData;
    QueueData txData;
    bool playOverlayActive=false;
    uint64_t lastPlayOverlayChangedTime=UTIL_get_time_us()-5*1000000;
    uint32_t sleepTimeSetupS=UI_MAIN_DEFAULT_SLEEP_TIME_S;//the setup sleeptime
    uint8_t resumePlayOnly=0;
    uint16_t queueWaitTime=10;
    int32_t searchId=-1;
    uint8_t extraFlags=0;

    SAVES_saveState_t UI_MAIN_saveState;
    memset(&UI_MAIN_saveState,0,sizeof(SAVES_saveState_t));
    memset(&UI_MAIN_settings,0,sizeof(SAVES_settings_t));

    char UI_MAIN_selectedFolderName[FF_FILE_PATH_MAX];//="Der Medicus Ein Roman von Noha Gordon";
    char UI_MAIN_selectedFileName[FF_FILE_PATH_MAX];
    char UI_MAIN_filePath[FF_FILE_PATH_MAX];
    uint8_t sleepOffSM=0;

    SDL_Thread *ffScanThread=NULL;
    QueueData fromFFScannerData;
    QueueData *fromFFScannerDataPtr;
    QueueData toFFScannerData;

    while(!quit){
        now=UTIL_get_time_us();
        if((now-UI_MAIN_offTimestamp)/1000000>300){//300s without playing or key strokes?->off
            quit=2; // quitting with 2 means power down device
        }
        if((sleepTimeOffTime!=0)&&(sleepTimeOffTime<now)){
            switch(sleepOffSM){
                case 0: saveFlag=1;sleepOffSM=1;//make sure current position is saved before stopping the player
                        break;
                case 1:
                        //SCREENS_switchingOff(0,SAVES_getUsedSpaceLevel(),SAVES_cleanOldBookmarks(1,&UI_MAIN_baseFolder[0]));
                        //stop the player
                        txData.type=QUEUE_DATA_SD_PLAY;
                        txData.sd_play_message.msgType=SD_PLAY_MSG_TYPE_PAUSE;
                        SD_PLAY_sendMessage(&txData);
                        SDL_Delay(100);//give the player some time to stop
                        quit=2; // quitting with 2 means power down device
                        break;
            }
        }

        if(UI_ELEMENTS_isDisplayOff()){
            queueWaitTime=100;
        }else{
            queueWaitTime=10;
        }
        switch(mainSM){
            case UI_MAIN_RUN_SM_INIT:

                    mainSM=UI_MAIN_RUN_SM_FOLDER_SCAN;
                    UI_MAIN_searchId=NULL;
                    UI_MAIN_searchString=NULL;
                    if(strlen(UI_MAIN_baseFolder)!=0){
                        //auto bookmark cleanup
                        int64_t spaceLeft=SAVES_getSpaceLeft();
                        if((spaceLeft>0)&&(spaceLeft<2)){
                            SAVES_cleanOldBookmarks(0,&UI_MAIN_baseFolder[0]);
                        }
                        UI_MAIN_settings.brightness=250;//default
                        if(SAVES_loadSettings(&UI_MAIN_settings)==0){
                            reducedMode=UI_MAIN_settings.reducedMode;
                            if(UI_MAIN_settings.brightness==0){
                                UI_MAIN_settings.brightness=250;
                            }
                            UI_ELEMENTS_setBrightness(UI_MAIN_settings.brightness);
                            UI_MAIN_volume=UI_MAIN_settings.volume;
                            if(UI_MAIN_settings.sleepTimeSetupS<60){
                                UI_MAIN_settings.sleepTimeSetupS=UI_MAIN_DEFAULT_SLEEP_TIME_S;
                            }
                            sleepTimeSetupS=UI_MAIN_settings.sleepTimeSetupS;
                            if(UI_MAIN_settings.lastFolderName[0]!=0){
                                UI_MAIN_searchString=&UI_MAIN_settings.lastFolderName[0];
                                UI_MAIN_searchId=&searchId;
                            }
                            if(UI_MAIN_settings.screenRotation){
                                UI_ELEMENTS_rotate(1);
                            }else{
                                UI_ELEMENTS_rotate(0);
                            }
                            DISPLAY_setFont(UI_MAIN_settings.fontNumber);
                            INPUT_switchAB(UI_MAIN_settings.abSwitch);
                        }
                        ffScanThread=SDL_CreateThread(UI_MAIN_scanSortTaskAllBooks, "UI_MAIN_scanSortTaskAllBooks",NULL);
                    }else{
                        SCREENS_noFolders();
                        SDL_Delay(2000);
                        quit=1;
                    }
                    break;
            case UI_MAIN_RUN_SM_FOLDER_SCAN:

                    fromFFScannerDataPtr=&fromFFScannerData;
                    queue_pop_timeout(&fromFFScanner,&fromFFScannerDataPtr,1);
                    if(fromFFScannerDataPtr!=NULL){
                        if(fromFFScannerData.ff_message.message==-1){//finished ok
                            if(UI_MAIN_amountOfBooks==0){//show empty folder symbol
                                SCREENS_noFolders();
                                SDL_Delay(3000);
                                mainSM=UI_MAIN_RUN_SM_INIT;//this state is not relly rechaed but will assure that the scan thread is closed
                                quit=1;
                            }else{
                                selectedFolder=1;
                                UI_MAIN_selectedFolderName[0]=0;
                                UI_MAIN_selectedFileName[0]=0;
                                mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                                extraFlags=0;
                                if(searchId>=0){//did we search for a folder that was last listened and found one?
                                    selectedFolder=searchId+1;
                                    strcpy(&UI_MAIN_selectedFolderName[0],&UI_MAIN_settings.lastFolderName[0]);
                                    UI_MAIN_folderPath[0]=0;
                                    strcpy(&UI_MAIN_folderPath[0],&UI_MAIN_baseFolder[0]);
                                    strcat(&UI_MAIN_folderPath[0],"/");
                                    strcat(&UI_MAIN_folderPath[0],&UI_MAIN_selectedFolderName[0]);
                                    if(SAVES_existsBookmark(&UI_MAIN_selectedFolderName[0],&UI_MAIN_saveState)==0){
                                        UI_MAIN_searchString=&UI_MAIN_saveState.fileName[0];
                                        UI_MAIN_searchId=&searchId;
                                    }
                                    if(ffScanThread!=NULL){
                                        int state=0;
                                        SDL_WaitThread(ffScanThread,&state);
                                    }
                                    ffScanThread=SDL_CreateThread(UI_MAIN_scanSortTaskOneBook, "UI_MAIN_scanSortTaskOneBook",NULL);
                                    mainSM=UI_MAIN_RUN_SM_BOOK_SCAN;
                                }else{
                                    UI_MAIN_searchString=NULL;
                                    UI_MAIN_searchId=NULL;
                                    searchId=-1;
                                }
                            }
                        }else if(fromFFScannerData.ff_message.message==-4){//scan canceled
                            mainSM=UI_MAIN_RUN_SM_INIT;
                            quit=1;
                        }else if(fromFFScannerData.ff_message.message>=0){
                            UI_MAIN_lastKeyPressedTime=now;
                            UI_MAIN_offTimestamp=now;
                            SCREENS_scanAllBooks(fromFFScannerData.ff_message.message);
                        }else{//some kind of error
                            mainSM=UI_MAIN_RUN_SM_INIT;
                            quit=1;
                        }
                        if((mainSM!=UI_MAIN_RUN_SM_FOLDER_SCAN)&&(ffScanThread!=NULL)){
                            int state=0;
                            SDL_WaitThread(ffScanThread,&state);
                            ffScanThread=NULL;
                        }
                    }
                    break;
            case UI_MAIN_RUN_SM_FOLDER_SELECTION:
                    if(selectedFolder==0){
                        SCREENS_setupSelect();
                    }else{
                        if(FF_getNameByID(&UI_MAIN_baseFolder[0],UI_MAIN_sortedBookIDs[selectedFolder-1],&UI_MAIN_selectedFolderName[0],0)!=0){
                            mainSM=UI_MAIN_RUN_SM_INIT;
                        }
                        if((extraFlags&0x80)==0){
                            UI_MAIN_folderPath[0]=0;
                            strcpy(&UI_MAIN_folderPath[0],&UI_MAIN_baseFolder[0]);
                            strcat(&UI_MAIN_folderPath[0],"/");
                            strcat(&UI_MAIN_folderPath[0],&UI_MAIN_selectedFolderName[0]);
                            if(SAVES_existsBookmark(&UI_MAIN_selectedFolderName[0],&UI_MAIN_saveState)==0){
                                UI_MAIN_searchString=&UI_MAIN_saveState.fileName[0];
                                UI_MAIN_searchId=&searchId;
                                extraFlags|=0x81;
                                if(UI_MAIN_saveState.finished&0x01){
                                    extraFlags|=0x02;
                                }
                            }
                        }
                        SCREENS_folderSelect(selectedFolder,UI_MAIN_amountOfBooks,&UI_MAIN_selectedFolderName[0],UI_MAIN_getWakeupTimer(),extraFlags&0x7F);
                    }
                    while(UI_MAIN_readKeyAndVLCQueue(1,queueWaitTime,&rxData)!=0){
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if(rxData.input_message.key==INPUT_KEY_DOWN_STRONG){
                                if(selectedFolder>10){
                                    selectedFolder-=10;
                                }else{
                                    selectedFolder=1;
                                }
                            }else{
                                if(selectedFolder>0){
                                    selectedFolder--;
                                }else{
                                    selectedFolder=UI_MAIN_amountOfBooks;
                                }
                            }
                            UI_ELEMENTS_textScrollyReset(2,0,12);
                            extraFlags=0;
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if(rxData.input_message.key==INPUT_KEY_UP_STRONG){
                                if(selectedFolder+10<UI_MAIN_amountOfBooks){
                                    selectedFolder+=10;
                                }else{
                                    selectedFolder=UI_MAIN_amountOfBooks;
                                }
                            }else{
                                if(selectedFolder<UI_MAIN_amountOfBooks){
                                    selectedFolder++;
                                }else{
                                    selectedFolder=0;
                                }
                            }
                            UI_ELEMENTS_textScrollyReset(2,0,12);
                            extraFlags=0;
                        }else if((rxData.input_message.key==INPUT_KEY_OK)||((reducedMode)&&(rxData.input_message.key==INPUT_KEY_BACK))){
                            if(selectedFolder==0){
                                mainSM=UI_MAIN_RUN_SM_SETUP_MENU;
                            }else{
                                ffScanThread=SDL_CreateThread(UI_MAIN_scanSortTaskOneBook, "UI_MAIN_scanSortTaskOneBook",NULL);
                                mainSM=UI_MAIN_RUN_SM_BOOK_SCAN;
                            }
                        }else if(rxData.input_message.key==INPUT_KEY_BACK_LONG){
                            quit=2; // quitting with 2 means power down device
                        }else if(rxData.input_message.key==INPUT_KEY_SELECT_AND_START){
                            quit=1;//allow manual emulation by using select here in this menu
                        }
                    }
                    break;
            case UI_MAIN_RUN_SM_SETUP_MENU:
                    if(UI_MAIN_setupMenu()!=0){
                        mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                        extraFlags=0;
                    }
                    break;

            case UI_MAIN_RUN_SM_BOOK_SCAN:
                    fromFFScannerDataPtr=&fromFFScannerData;
                    queue_pop_timeout(&fromFFScanner,&fromFFScannerDataPtr,1);
                    if(fromFFScannerDataPtr!=NULL){

                        if(fromFFScannerData.ff_message.message==-1){//finished ok
                            if(UI_MAIN_amountOfFiles==0){//no files found, go back to folder selection
                                mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                                extraFlags=0;
                            }else{
                                pauseMode=0;//default chapter selection for a newly selected book
                                selectedFile=1;
                                oldSelectedFile=selectedFile;
                                currentPlayMinute=0;
                                currentPlaySecond=0;
                                allPlayMinute=0;
                                allPlaySecond=0;
                                newPlayMinute=0;
                                currentPlaySpeed=100;
                                currentEqualizer=0;
                                currentRepeatMode=0;
                                currentFinishedFlag=0;
                                mainSM=UI_MAIN_RUN_SM_PAUSED;
                                if(searchId>=0){//did we search for a filename that was last listened and found one?
                                    strcpy(&UI_MAIN_selectedFileName[0],&UI_MAIN_saveState.fileName[0]);
                                    selectedFile=searchId+1;
                                    oldSelectedFile=selectedFile;
                                    currentPlayMinute=UI_MAIN_saveState.playMinute;
                                    newPlayMinute=currentPlayMinute;
                                    currentPlaySecond=UI_MAIN_saveState.playSecond;
                                    currentPlaySpeed=UI_MAIN_saveState.playSpeed;
                                    if(currentPlaySpeed<50) currentPlaySpeed=100;
                                    if(currentPlaySpeed>300) currentPlaySpeed=100;
                                    currentEqualizer=UI_MAIN_saveState.equalizer;
                                    currentRepeatMode=UI_MAIN_saveState.repeatMode;
                                    currentFinishedFlag=UI_MAIN_saveState.finished;
                                    pauseMode=1;//go to time selection for bookmarks
                                }
                                //find track length to be able to seek correctly
                                if(FF_getNameByID(&UI_MAIN_folderPath[0],UI_MAIN_sortedFileIDs[selectedFile-1],&UI_MAIN_selectedFileName[0],1)==0){
                                    strcpy(&UI_MAIN_filePath[0],&UI_MAIN_folderPath[0]);
                                    strcat(&UI_MAIN_filePath[0],"/");
                                    strcat(&UI_MAIN_filePath[0],&UI_MAIN_selectedFileName[0]);

                                    int64_t rt=SD_PLAY_getRuntime(&UI_MAIN_filePath[0]);
                                    if(rt>0){
                                        allPlaySecond=(rt/1000)%60;
                                        allPlayMinute=(rt/1000)/60;
                                    }else{
                                        allPlaySecond=1;
                                        allPlayMinute=0;
                                    }
                                }
                                if((UI_MAIN_amountOfFiles==1)||(reducedMode)){
                                    reducedModeLastTimestamp=now;
                                    pauseMode=1;//no need for chapter selection if only one track exists, also reducedMode switches right to time seek mode
                                    newPlayMinute=currentPlayMinute;//but make sure the blinking adjustable position value is the same as the current position
                                }
                                SD_PLAY_setEqualizer(currentEqualizer);
                                UI_MAIN_lastKeyPressedTime=now;//switch screen on if it was off before because of a long scan time
                                UI_MAIN_searchString=NULL;
                                UI_MAIN_searchId=NULL;
                                searchId=-1;
                            }
                        }else if(fromFFScannerData.ff_message.message==-4){//scan canceled
                            mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                            extraFlags=0;
                        }else if(fromFFScannerData.ff_message.message>=0){
                            UI_MAIN_offTimestamp=now;
                            SCREENS_scanOneBook(fromFFScannerData.ff_message.message);
                        }else{//some kind of error
                            mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                            extraFlags=0;
                        }
                    }
                    if(UI_MAIN_readKeyAndVLCQueue(1,queueWaitTime,&rxData)!=0){//wait for incoming key messages
                        if((rxData.input_message.key==INPUT_KEY_BACK)||(rxData.input_message.key==INPUT_KEY_OK)){
                            toFFScannerData.type=-QUEUE_DATA_FF;
                            toFFScannerData.ff_message.message=-1;
                            queue_push(&toFFScanner,&toFFScannerData);
                        }
                    }
                    if(mainSM!=UI_MAIN_RUN_SM_BOOK_SCAN){
                        int state=0;
                        SDL_WaitThread(ffScanThread,&state);
                        ffScanThread=NULL;
                    }
                    break;
            case UI_MAIN_RUN_SM_PAUSED:
                    playOverlayMode=0;
                    if(pauseMode==0){//with chapter selection
                        percent=(currentPlayMinute*60+currentPlaySecond)*100/(allPlayMinute*60+allPlaySecond);
                        SCREENS_pause0(selectedFile,UI_MAIN_amountOfFiles,&UI_MAIN_selectedFolderName[0],currentPlayMinute,currentPlaySecond,percent,UI_MAIN_timeDiffNowS(sleepTimeOffTime));
                        if(reducedMode){
                            if((UTIL_get_time_us()-reducedModeLastTimestamp)/1000>10000){//go back to book selection after 10s in chapter selection mode
                                reducedModeLastTimestamp=now;
                                mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                                resumePlayOnly=0;
                                extraFlags=0;
                            }
                        }
                    }else if(pauseMode==1){//with time selection
                        percent=(newPlayMinute*60+currentPlaySecond)*100/(allPlayMinute*60+allPlaySecond);
                        SCREENS_pause1(selectedFile,UI_MAIN_amountOfFiles,&UI_MAIN_selectedFolderName[0],newPlayMinute,currentPlaySecond,percent,UI_MAIN_timeDiffNowS(sleepTimeOffTime));
                        if(reducedMode){
                            if((UTIL_get_time_us()-reducedModeLastTimestamp)/1000>10000){//switch to chapter selection or book selection after 10s in time selection mode
                                reducedModeLastTimestamp=now;
                                if(UI_MAIN_amountOfFiles==1){
                                    mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                                    resumePlayOnly=0;
                                    extraFlags=0;
                                }else{
                                    pauseMode=0;
                                }
                            }
                        }
                    }
                    while(UI_MAIN_readKeyAndVLCQueue(1,queueWaitTime,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) reducedModeLastTimestamp=now;
                        if(pauseMode==0){//with chapter selection
                            if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                                if(selectedFile>1){
                                    selectedFile--;
                                }else{
                                    selectedFile=UI_MAIN_amountOfFiles;
                                }
                            }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                                if(selectedFile<UI_MAIN_amountOfFiles){
                                    selectedFile++;
                                }else{
                                    selectedFile=1;
                                }
                            }else if((rxData.input_message.key==INPUT_KEY_OK)||((reducedMode)&&(rxData.input_message.key==INPUT_KEY_BACK))){
                                mainSM=UI_MAIN_RUN_SM_PLAY_INIT;
                                if(oldSelectedFile!=selectedFile){
                                    resumePlayOnly=0;
                                    currentPlayMinute=0;
                                    currentPlaySecond=0;
                                    oldSelectedFile=selectedFile;
                                }
                            }else if(rxData.input_message.key==INPUT_KEY_BACK){
                                resumePlayOnly=0;
                                mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                                extraFlags=0;
                            }
                        }else if(pauseMode==1){//with time selection
                            if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                                resumePlayOnly=0;
                                if(currentPlaySecond!=0){
                                    currentPlaySecond=0;
                                }else{
                                    if(rxData.input_message.key==INPUT_KEY_DOWN){
                                        if(newPlayMinute!=0){
                                            newPlayMinute--;
                                        }
                                    }else{
                                        if(newPlayMinute>=30){
                                            newPlayMinute-=30;
                                        }else{
                                            newPlayMinute=0;
                                        }
                                    }
                                }
                            }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                                resumePlayOnly=0;
                                currentPlaySecond=0;
                                if(rxData.input_message.key==INPUT_KEY_UP){
                                    newPlayMinute++;
                                }else{
                                    newPlayMinute+=30;
                                }
                                if(newPlayMinute>allPlayMinute) newPlayMinute=allPlayMinute;
                            }else if((rxData.input_message.key==INPUT_KEY_OK)||((reducedMode)&&(rxData.input_message.key==INPUT_KEY_BACK))){
                                mainSM=UI_MAIN_RUN_SM_PLAY_INIT;
                                currentPlayMinute=newPlayMinute;
                            }else if(rxData.input_message.key==INPUT_KEY_BACK){
                                if(pauseMode==1){
                                    if(UI_MAIN_amountOfFiles==1){
                                        resumePlayOnly=0;
                                        mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                                        extraFlags=0;
                                    }else{
                                        pauseMode=0;
                                    }
                                }else{
                                    resumePlayOnly=0;
                                    mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                                    extraFlags=0;
                                }
                            }
                        }
                        if(rxData.input_message.key==INPUT_KEY_SELECT_AND_START){
                            quit=1;
                        }
                    }
                    break;
            case UI_MAIN_RUN_SM_PLAY_INIT:
                    if(FF_getNameByID(&UI_MAIN_folderPath[0],UI_MAIN_sortedFileIDs[selectedFile-1],&UI_MAIN_selectedFileName[0],1)==0){
                        strcpy(&UI_MAIN_filePath[0],&UI_MAIN_folderPath[0]);
                        strcat(&UI_MAIN_filePath[0],"/");
                        strcat(&UI_MAIN_filePath[0],&UI_MAIN_selectedFileName[0]);

                        if(resumePlayOnly!=0){
                            txData.type=QUEUE_DATA_SD_PLAY;
                            txData.sd_play_message.msgType=SD_PLAY_MSG_TYPE_RESUME;
                        }else{
                            txData.type=QUEUE_DATA_SD_PLAY;
                            txData.sd_play_message.msgType=SD_PLAY_MSG_TYPE_START_PLAY;
                            txData.sd_play_message.msgData=&UI_MAIN_filePath[0];
                            txData.sd_play_message.startPosition=(currentPlayMinute*60+currentPlaySecond)*1000;
                        }
                        SD_PLAY_sendMessage(&txData);

                        lastAutoSave=now;

                        int64_t rt=SD_PLAY_getRuntime(&UI_MAIN_filePath[0]);
                        if(rt>0){
                            allPlaySecond=(rt/1000)%60;
                            allPlayMinute=(rt/1000)/60;
                        }

                        SD_PLAY_setVolume(UI_MAIN_volume);
                        SD_PLAY_setPlaySpeed(currentPlaySpeed);
                        mainSM=UI_MAIN_RUN_SM_PLAYING;
                    }else{//something went wrong
                        mainSM=UI_MAIN_RUN_SM_FOLDER_SELECTION;
                        extraFlags=0;
                    }
                    pauseMode=0;
                    saveFlag=0;
                    resumePlayOnly=0;

                    break;
            case UI_MAIN_RUN_SM_PLAYING:
                    if(!UI_ELEMENTS_isDisplayOff()){
                        if(((now-lastPlayOverlayChangedTime)/1000)<3000){
                            SCREENS_playOverlay(playOverlayMode,UI_MAIN_volume,currentPlaySpeed,currentEqualizer,currentRepeatMode,sleepTimeSetupS,sleepTimeOffTime);
                        }else{
                            playOverlayActive=false;
                            if(((now-lastPlayOverlayChangedTime)/1000)>10000){//wait longer for the overlay to switch back to volume setting first
                                playOverlayMode=0;
                            }
                            if(playOverlayMode==0){
                                SCREENS_play(selectedFile,UI_MAIN_amountOfFiles,&UI_MAIN_selectedFolderName[0],currentPlayMinute,currentPlaySecond,percent,currentRepeatMode,allPlayMinute,allPlaySecond,UI_MAIN_timeDiffNowS(sleepTimeOffTime),0);
                            }else{
                                SCREENS_play(selectedFile,UI_MAIN_amountOfFiles,&UI_MAIN_selectedFolderName[0],currentPlayMinute,currentPlaySecond,percent,currentRepeatMode,allPlayMinute,allPlaySecond,UI_MAIN_timeDiffNowS(sleepTimeOffTime),(now-lastPlayOverlayChangedTime)/1000);
                            }
                        }
                    }
                    uint8_t cancel=0;
                    do{
                        if(UI_MAIN_readKeyAndVLCQueue(3,queueWaitTime,&rxData)!=0){
                            cancel=0;
                            if(rxData.type==QUEUE_DATA_SD_PLAY){
                                //lastSdPlayMessage=now;
                                if(rxData.sd_play_message.msgType==SD_PLAY_MSG_TYPE_STOPPED_NO_ERROR){//track finished
                                    //skipped=0;
                                    currentPlayMinute=0;
                                    currentPlaySecond=0;
                                    selectedFile++;
                                    mainSM=UI_MAIN_RUN_SM_PLAY_INIT;
                                    if(selectedFile>UI_MAIN_amountOfFiles){//book finished, reset everything to start
                                        selectedFile=1;
                                        if(currentRepeatMode&UI_MAIN_REPEAT_FOLDER){
                                            mainSM=UI_MAIN_RUN_SM_PLAY_INIT;//go on if we should repeat
                                        }else{
                                            mainSM=UI_MAIN_RUN_SM_PAUSED;
                                            if(reducedMode) reducedModeLastTimestamp=now;
                                        }
                                    }
                                    oldSelectedFile=selectedFile;
                                }else if((rxData.sd_play_message.msgType==SD_PLAY_MSG_TYPE_PAUSED)){//user pressed pause
                                    mainSM=UI_MAIN_RUN_SM_PAUSED;
                                    if(reducedMode) reducedModeLastTimestamp=now;
                                    saveFlag=1;
                                    if(selectedFile==UI_MAIN_amountOfFiles){//listening to last track of book
                                        if(((currentPlayMinute*60+currentPlaySecond)*100/(allPlayMinute*60+allPlaySecond))>95){
                                            currentFinishedFlag=1;
                                        }else{
                                            currentFinishedFlag=0;
                                        }
                                    }
                                }else if(rxData.sd_play_message.msgType==SD_PLAY_MSG_TYPE_STOPPED_ERROR){
                                    mainSM=UI_MAIN_RUN_SM_PAUSED;
                                    if(reducedMode) reducedModeLastTimestamp=now;
                                }else if(rxData.sd_play_message.msgType == SD_PLAY_MSG_TYPE_FILEPOS_STATE){
                                    currentPlaySecond=(rxData.sd_play_message.currentPosition/1000)%60;
                                    currentPlayMinute=(rxData.sd_play_message.currentPosition/1000)/60;
                                    allPlaySecond=(rxData.sd_play_message.trackLength/1000)%60;
                                    allPlayMinute=(rxData.sd_play_message.trackLength/1000)/60;
                                    percent=(currentPlayMinute*60+currentPlaySecond)*100/(allPlayMinute*60+allPlaySecond);
                                    newPlayMinute=currentPlayMinute;
                                    if(((now-lastAutoSave)/1000000)>60){
                                        saveFlag=1;
                                        lastAutoSave=now;
                                    }
                                }
                            }
                            if(rxData.type==QUEUE_DATA_INPUT){//wait for incoming key messages
                                if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                                    if(playOverlayActive){//first turn should not already do some action
                                        switch(playOverlayMode){
                                            case 0:
                                                        if(rxData.input_message.key==INPUT_KEY_DOWN){
                                                            if(UI_MAIN_volume>=0) UI_MAIN_volume-=1;
                                                        }else{
                                                            if(UI_MAIN_volume>=0) UI_MAIN_volume-=5;
                                                        }
                                                        if(UI_MAIN_volume<0) UI_MAIN_volume=0;
                                                        if(UI_MAIN_volume>150) UI_MAIN_volume=150;
                                                        SD_PLAY_setVolume(UI_MAIN_volume);
                                                        break;
                                            case 1:
                                                        if(sleepTimeSetupS>60){
                                                            if(sleepTimeOffTime!=0){
                                                                sleepTimeSetupS-=60;
                                                            }
                                                            sleepTimeOffTime=now+(uint64_t)((uint64_t)sleepTimeSetupS*(uint64_t)1000000);
                                                        }else{
                                                            sleepTimeSetupS=0;
                                                            sleepTimeOffTime=0;
                                                        }
                                                        break;
                                            case 2:
                                                        if(currentPlaySpeed>50){
                                                            currentPlaySpeed-=5;
                                                        }
                                                        SD_PLAY_setPlaySpeed(currentPlaySpeed);
                                                        break;
                                            case 3:     if(currentEqualizer>0){
                                                            currentEqualizer--;
                                                        }
                                                        SD_PLAY_setEqualizer(currentEqualizer);
                                                        break;
                                            case 4:     if(currentRepeatMode&UI_MAIN_REPEAT_FOLDER){
                                                            currentRepeatMode&=~UI_MAIN_REPEAT_FOLDER;
                                                        }else{
                                                            currentRepeatMode|=UI_MAIN_REPEAT_FOLDER;
                                                        }
                                                        break;
                                        }
                                    }
                                    lastPlayOverlayChangedTime=now;playOverlayActive=true;
                                }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                                    if(playOverlayActive){//first turn should not already do some action
                                        switch(playOverlayMode){
                                            case 0:
                                                        if(rxData.input_message.key==INPUT_KEY_UP){
                                                            if(UI_MAIN_volume<=150) UI_MAIN_volume+=1;
                                                        }else{
                                                            if(UI_MAIN_volume<=150) UI_MAIN_volume+=5;
                                                        }
                                                        if(UI_MAIN_volume<0) UI_MAIN_volume=0;
                                                        if(UI_MAIN_volume>150) UI_MAIN_volume=150;
                                                        SD_PLAY_setVolume(UI_MAIN_volume);
                                                        break;
                                            case 1:
                                                        if(sleepTimeSetupS<480*60){
                                                            if(sleepTimeOffTime!=0){
                                                                sleepTimeSetupS+=60;
                                                            }
                                                            if(sleepTimeSetupS==0) sleepTimeSetupS=60;
                                                            sleepTimeOffTime=now+(uint64_t)((uint64_t)sleepTimeSetupS*(uint64_t)1000000);
                                                        }else{
                                                            sleepTimeSetupS=480*60;
                                                            sleepTimeOffTime=now+(uint64_t)((uint64_t)sleepTimeSetupS*(uint64_t)1000000);
                                                        }
                                                        break;
                                            case 2:
                                                        if(currentPlaySpeed<250){
                                                            currentPlaySpeed+=5;
                                                        }
                                                        SD_PLAY_setPlaySpeed(currentPlaySpeed);
                                                        break;
                                            case 3:     if(currentEqualizer<4){
                                                            currentEqualizer++;
                                                        }
                                                        SD_PLAY_setEqualizer(currentEqualizer);
                                                        break;
                                            case 4:     if(currentRepeatMode&UI_MAIN_REPEAT_FOLDER){
                                                            currentRepeatMode&=~UI_MAIN_REPEAT_FOLDER;
                                                        }else{
                                                            currentRepeatMode|=UI_MAIN_REPEAT_FOLDER;
                                                        }
                                                        break;
                                        }
                                    }
                                    lastPlayOverlayChangedTime=now;playOverlayActive=true;
                                }else if((rxData.input_message.key==INPUT_KEY_OK)||((reducedMode)&&(rxData.input_message.key==INPUT_KEY_BACK))){
                                    if(playOverlayActive){
                                        if(playOverlayMode<4){
                                            playOverlayMode++;
                                        }else{
                                            playOverlayMode=0;
                                        }
                                        if(reducedMode) playOverlayMode=0;//stay in volume mode in reduced mode
                                        lastPlayOverlayChangedTime=now;playOverlayActive=true;
                                    }else{
                                        txData.type=QUEUE_DATA_SD_PLAY;
                                        txData.sd_play_message.msgType=SD_PLAY_MSG_TYPE_PAUSE;
                                        SD_PLAY_sendMessage(&txData);
                                        resumePlayOnly=1;
                                        pauseMode=1;
                                    }
                                }else if(rxData.input_message.key==INPUT_KEY_BACK){
                                    if(playOverlayActive){
                                        lastPlayOverlayChangedTime=0;playOverlayActive=false;
                                    }else{
                                        txData.type=QUEUE_DATA_SD_PLAY;
                                        txData.sd_play_message.msgType=SD_PLAY_MSG_TYPE_PAUSE;
                                        SD_PLAY_sendMessage(&txData);
                                        resumePlayOnly=1;
                                        pauseMode=1;
                                    }
                                }else if(rxData.input_message.key==INPUT_KEY_SELECT){
                                    if((!playOverlayActive)&&(!reducedMode)){//only in normal play mode and when reduced mode is off
                                        if(sleepTimeSetupS==0) sleepTimeSetupS=60;
                                        if(sleepTimeOffTime!=0){
                                            sleepTimeOffTime=0;
                                        }else{
                                            sleepTimeOffTime=now+(uint64_t)((uint64_t)sleepTimeSetupS*(uint64_t)1000000);
                                        }
                                    }
                                }else if(rxData.input_message.key==INPUT_KEY_SELECT_AND_START){
                                    txData.type=QUEUE_DATA_SD_PLAY;
                                    txData.sd_play_message.msgType=SD_PLAY_MSG_TYPE_PAUSE;
                                    SD_PLAY_sendMessage(&txData);
                                    resumePlayOnly=1;
                                    pauseMode=1;
                                    saveFlag=1;
                                    quit=1;
                                }
                            }
                        }else{
                            cancel=1;
                        }
                    }while(cancel==0);
                    if((UI_MAIN_timeDiffNowS(sleepTimeOffTime)>0)&&(UI_MAIN_timeDiffNowS(sleepTimeOffTime)<=10)){//volume fade out if sleep timer nears end
                        SD_PLAY_setVolume(UI_MAIN_volume-(UI_MAIN_volume/UI_MAIN_timeDiffNowS(sleepTimeOffTime)));
                    }

                    /*if((now-lastSdPlayMessage)/1000000>2){//2s no message from sd play
                        //ESP_LOGE(UI_MAIN_LOG_TAG,"PLAYTIMEOUT");
                        mainSM=UI_MAIN_RUN_SM_PAUSED;
                        //if(reducedMode) reducedModeLastTimestamp=UTIL_get_time_us();
                    }*/

                    if(saveFlag){
                        memset(&UI_MAIN_saveState,0,sizeof(SAVES_saveState_t));
                        saveFlag=0;
                        if(currentPlaySecond>=5){
                            UI_MAIN_saveState.playMinute=currentPlayMinute;
                            UI_MAIN_saveState.playSecond=currentPlaySecond-5;
                        }else{
                            if(currentPlayMinute>0){
                                UI_MAIN_saveState.playMinute=currentPlayMinute-1;
                                UI_MAIN_saveState.playSecond=(60-(5-currentPlaySecond))%60;
                            }else{
                                UI_MAIN_saveState.playMinute=0;
                                UI_MAIN_saveState.playSecond=0;
                            }
                        }
                        UI_MAIN_saveState.playSpeed=currentPlaySpeed;
                        UI_MAIN_saveState.equalizer=currentEqualizer;
                        UI_MAIN_saveState.repeatMode=currentRepeatMode;
                        UI_MAIN_saveState.finished=currentFinishedFlag;
                        strcpy(&UI_MAIN_saveState.fileName[0],&UI_MAIN_selectedFileName[0]);
                        SAVES_saveBookmark(&UI_MAIN_selectedFolderName[0],&UI_MAIN_saveState);
                        UI_MAIN_settings.volume=UI_MAIN_volume;
                        UI_MAIN_settings.sleepTimeSetupS=sleepTimeSetupS;
                        strcpy(&UI_MAIN_settings.lastFolderName[0],&UI_MAIN_selectedFolderName[0]);
                        SAVES_saveSettings(&UI_MAIN_settings);
                    }
                    break;
        }//of switch
        if(!UI_ELEMENTS_isDisplayOff()){
            if(reducedMode==1){
                if((now>UI_MAIN_lastKeyPressedTime)&&(now-UI_MAIN_lastKeyPressedTime)/1000>60000){
                    UI_ELEMENTS_displayOff();
                    UI_ELEMENTS_setBrightness(0);
                }
            }else{
                if((now>UI_MAIN_lastKeyPressedTime)&&(now-UI_MAIN_lastKeyPressedTime)/1000>15000){
                    UI_ELEMENTS_displayOff();
                    UI_ELEMENTS_setBrightness(0);
                }
            }
        }else{
            if(UI_MAIN_settings.screenSaverMode==1){
                DISPLAY_screenSaver();
            }
        }
	}//of while running

    if(governor[0]!=0){
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"Setting CPU governor back to: %s.\n",&governor[0]);
        UTIL_setCPUGovernor(&governor[0]);
    }

    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"Finishing.\n");
    if(quit==2) SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,SDL_LOG_PRIORITY_INFO,"Requesting a shutdown.\n");

    SD_PLAY_close();
    DISPLAY_cls();
    DISPLAY_update();
    INPUT_close();
    if(!((quit==2)&&(UI_ELEMENTS_isDisplayOff()))){
        DISPLAY_setRawBrightness(UI_MAIN_originalBacklight);
    }
    DISPLAY_close();
    return quit;
}


typedef struct UI_MAIN_setupMenuDataStruct{
    uint8_t sm;
    uint8_t selectedSub;
    uint64_t lastMinuteChangedTime;
    uint64_t wakeupTimeSetupS;
    int32_t numberOfBookmarks;
}UI_MAIN_setupMenuData_t;
UI_MAIN_setupMenuData_t UI_MAIN_setupMenuData;
uint64_t setupMenuReducedModeLastTimestamp=0;
#define UI_MAIN_SETUP_MENU_SM_INIT 0
#define UI_MAIN_SETUP_MENU_SUB_SELECTION 5
#define UI_MAIN_SETUP_MENU_SM_WAKEUP_TIMER 10
#define UI_MAIN_SETUP_MENU_SM_SCREEN_SETUP 15
#define UI_MAIN_SETUP_MENU_SM_SCREEN_BRIGHTNESS 20
#define UI_MAIN_SETUP_MENU_SM_SCREEN_SAVER 22
#define UI_MAIN_SETUP_MENU_SM_FONT 25
#define UI_MAIN_SETUP_MENU_SM_AB_SWITCH 30
#define UI_MAIN_SETUP_MENU_SM_BOOKMARK_DELETE 35
#define UI_MAIN_SETUP_MENU_SM_REDUCED_MODE 40
#define UI_MAIN_SETUP_MENU_SM_CLEANUP 250

/**
  * @brief displays and handles all the setup menu settings
  *
  * @return 0=setup still busy, >0 setup finished
  *
  */
uint8_t UI_MAIN_setupMenu(){
    uint64_t now=UTIL_get_time_us(); //make now available for all states of the state machine
    QueueData rxData;

    switch(UI_MAIN_setupMenuData.sm){
        case UI_MAIN_SETUP_MENU_SM_INIT:
                    setupMenuReducedModeLastTimestamp=now;
                    UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                    UI_MAIN_setupMenuData.lastMinuteChangedTime=now-5*1000000;
                    memset(&rxData,0,sizeof(QueueData));
                    UI_MAIN_setupMenuData.numberOfBookmarks=SAVES_cleanOldBookmarks(1,&UI_MAIN_baseFolder[0]);
                    break;
        case UI_MAIN_SETUP_MENU_SUB_SELECTION:
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if(UI_MAIN_setupMenuData.selectedSub==0){
                                UI_MAIN_setupMenuData.selectedSub=6;
                            }else{
                                UI_MAIN_setupMenuData.selectedSub--;
                            }
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if(UI_MAIN_setupMenuData.selectedSub==6){
                                UI_MAIN_setupMenuData.selectedSub=0;
                            }else{
                                UI_MAIN_setupMenuData.selectedSub++;
                            }
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            switch(UI_MAIN_setupMenuData.selectedSub){
                                /*case 0: //wakeup timer
                                        UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_WAKEUP_TIMER;
                                        break;*/
                                case 0: //screen rotation
                                        UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_SCREEN_SETUP;
                                        break;
                                case 1: //screen rotation
                                        UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_SCREEN_BRIGHTNESS;
                                        break;
                                case 2: //screen saver
                                        UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_SCREEN_SAVER;
                                        break;
                                case 3: //font setup
                                        UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_FONT;
                                        break;
                                case 4: //rotary encoder speed
                                        UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_AB_SWITCH;
                                        break;
                                case 5: //bookmark deletion
                                        UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_BOOKMARK_DELETE;
                                        break;
                                case 6: //reduced functionality mode
                                        UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_REDUCED_MODE;
                                        break;
                            }
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_CLEANUP;
                        }
                    }
                    switch(UI_MAIN_setupMenuData.selectedSub){
                        /*case 0: //wakeup timer
                                SCREENS_wakeupTimer(UI_MAIN_setupMenuData.wakeupTimeSetupS,0);
                                break;*/
                        case 0: //screen rotation
                                SCREENS_screenSetup(UI_MAIN_settings.screenRotation,0);
                                break;
                        case 1: //screen brightness
                                SCREENS_screenBrightnessSetup(UI_MAIN_settings.brightness,0);
                                break;
                        case 2: //screen saver
                                SCREENS_screenSaverSetup(UI_MAIN_settings.screenSaverMode,0);
                                break;
                        case 3: //font setup
                                SCREENS_fontSetup(UI_MAIN_settings.fontNumber,0);
                                break;
                        case 4: //rotary encoder speed
                                SCREENS_abSwitch(UI_MAIN_settings.abSwitch,0);
                                break;
                        case 5: //bookmark deletion
                                SCREENS_bookmarkDeletionSetup(UI_MAIN_setupMenuData.numberOfBookmarks,0);
                                break;
                        case 6: //reduced functionality mode
                                SCREENS_reducedModeSetup(UI_MAIN_settings.reducedMode,0);
                                break;

                    }
                    UI_ELEMENTS_numberSelect(0,0,UI_MAIN_setupMenuData.selectedSub+1,7,1);
                    UI_ELEMENTS_update();
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//save settings in reduced mode if nothing was selected after 10s
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_CLEANUP;
                        }
                    }
                    break;
        case UI_MAIN_SETUP_MENU_SM_WAKEUP_TIMER:
                    SCREENS_wakeupTimer(UI_MAIN_setupMenuData.wakeupTimeSetupS,1);
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if((now-UI_MAIN_setupMenuData.lastMinuteChangedTime)>50000){
                                if(UI_MAIN_setupMenuData.wakeupTimeSetupS>60){
                                    UI_MAIN_setupMenuData.wakeupTimeSetupS-=60;
                                }else{
                                    UI_MAIN_setupMenuData.wakeupTimeSetupS=60;
                                }
                            }else{
                                if(UI_MAIN_setupMenuData.wakeupTimeSetupS>60*5){
                                    UI_MAIN_setupMenuData.wakeupTimeSetupS-=60*5;
                                }else{
                                    UI_MAIN_setupMenuData.wakeupTimeSetupS=60;
                                }
                            }
                            if(UI_MAIN_setupMenuData.wakeupTimeSetupS<60) UI_MAIN_setupMenuData.wakeupTimeSetupS=60;
                            UI_MAIN_setupMenuData.lastMinuteChangedTime=now;
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if((now-UI_MAIN_setupMenuData.lastMinuteChangedTime)>50000){
                                UI_MAIN_setupMenuData.wakeupTimeSetupS+=60;
                            }else{
                                UI_MAIN_setupMenuData.wakeupTimeSetupS+=60*5;
                            }
                            if(UI_MAIN_setupMenuData.wakeupTimeSetupS>3600*24) UI_MAIN_setupMenuData.wakeupTimeSetupS=3600*24;
                            UI_MAIN_setupMenuData.lastMinuteChangedTime=now;
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            UI_MAIN_setWakeupTimer(UI_MAIN_setupMenuData.wakeupTimeSetupS);
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setWakeupTimer(0);
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//go back after 10s no input in reduced mode
                            setupMenuReducedModeLastTimestamp=now;
                            UI_MAIN_setWakeupTimer(0);
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    break;
        case UI_MAIN_SETUP_MENU_SM_SCREEN_SETUP:
                    SCREENS_screenSetup(UI_MAIN_settings.screenRotation,1);
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if(UI_MAIN_settings.screenRotation){
                                UI_MAIN_settings.screenRotation=0;
                            }else{
                                UI_MAIN_settings.screenRotation=1;
                            }
                            UI_ELEMENTS_rotate(UI_MAIN_settings.screenRotation);
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if(UI_MAIN_settings.screenRotation){
                                UI_MAIN_settings.screenRotation=0;
                            }else{
                                UI_MAIN_settings.screenRotation=1;
                            }
                            UI_ELEMENTS_rotate(UI_MAIN_settings.screenRotation);
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//go back after 10s no input in reduced mode
                            setupMenuReducedModeLastTimestamp=now;
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    break;
        case UI_MAIN_SETUP_MENU_SM_SCREEN_BRIGHTNESS:
                    SCREENS_screenBrightnessSetup(UI_MAIN_settings.brightness,1);
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if(rxData.input_message.key==INPUT_KEY_DOWN_STRONG){
                                if(UI_MAIN_settings.brightness>5){
                                    UI_MAIN_settings.brightness-=5;
                                }else{
                                    UI_MAIN_settings.brightness=1;
                                }
                            }else{
                                if(UI_MAIN_settings.brightness>1){
                                    UI_MAIN_settings.brightness-=1;
                                }else{
                                    UI_MAIN_settings.brightness=1;
                                }
                            }
                            UI_ELEMENTS_setBrightness(UI_MAIN_settings.brightness);
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if(rxData.input_message.key==INPUT_KEY_UP_STRONG){
                                if(UI_MAIN_settings.brightness<=250){
                                    UI_MAIN_settings.brightness+=5;
                                }else{
                                    UI_MAIN_settings.brightness=255;
                                }
                            }else{
                                if(UI_MAIN_settings.brightness<255){
                                    UI_MAIN_settings.brightness+=1;
                                }else{
                                    UI_MAIN_settings.brightness=255;
                                }
                            }
                            UI_ELEMENTS_setBrightness(UI_MAIN_settings.brightness);
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//go back after 10s no input in reduced mode
                            setupMenuReducedModeLastTimestamp=now;
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    break;
        case UI_MAIN_SETUP_MENU_SM_SCREEN_SAVER:
                    SCREENS_screenSaverSetup(UI_MAIN_settings.screenSaverMode,1);
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if(UI_MAIN_settings.screenSaverMode==0){
                                UI_MAIN_settings.screenSaverMode=1;
                            }else{
                                UI_MAIN_settings.screenSaverMode=0;
                            }
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if(UI_MAIN_settings.screenSaverMode==0){
                                UI_MAIN_settings.screenSaverMode=1;
                            }else{
                                UI_MAIN_settings.screenSaverMode=0;
                            }
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//go back after 10s no input in reduced mode
                            setupMenuReducedModeLastTimestamp=now;
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    break;
        case UI_MAIN_SETUP_MENU_SM_FONT:
                    SCREENS_fontSetup(UI_MAIN_settings.fontNumber,1);
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if(DISPLAY_setFont(UI_MAIN_settings.fontNumber-1)==0){
                                UI_MAIN_settings.fontNumber--;
                                DISPLAY_clearCache();//clear grid cache so that font is directly displayed
                            }
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if(DISPLAY_setFont(UI_MAIN_settings.fontNumber+1)==0){
                                UI_MAIN_settings.fontNumber++;
                                DISPLAY_clearCache();//clear grid cache so that font is directly displayed
                            }
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//go back after 10s no input in reduced mode
                            setupMenuReducedModeLastTimestamp=now;
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    break;
        case UI_MAIN_SETUP_MENU_SM_AB_SWITCH:
                    SCREENS_abSwitch(UI_MAIN_settings.abSwitch,1);
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if(UI_MAIN_settings.abSwitch==0){
                                UI_MAIN_settings.abSwitch=1;
                            }else{
                                UI_MAIN_settings.abSwitch=0;
                            }
                            INPUT_switchAB(UI_MAIN_settings.abSwitch);
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if(UI_MAIN_settings.abSwitch==0){
                                UI_MAIN_settings.abSwitch=1;
                            }else{
                                UI_MAIN_settings.abSwitch=0;
                            }
                            INPUT_switchAB(UI_MAIN_settings.abSwitch);
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//go back after 10s no input in reduced mode
                            setupMenuReducedModeLastTimestamp=now;
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    break;
        case UI_MAIN_SETUP_MENU_SM_BOOKMARK_DELETE:
                    SCREENS_bookmarkDeletionSetup(UI_MAIN_setupMenuData.numberOfBookmarks,1);
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            SAVES_cleanOldBookmarks(15,&UI_MAIN_baseFolder[0]);
                            UI_MAIN_setupMenuData.numberOfBookmarks=SAVES_cleanOldBookmarks(1,&UI_MAIN_baseFolder[0]);
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//go back after 10s no input in reduced mode
                            setupMenuReducedModeLastTimestamp=now;
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    break;
        case UI_MAIN_SETUP_MENU_SM_CLEANUP:
                    UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SM_INIT;
                    SAVES_saveSettings(&UI_MAIN_settings);
                    reducedMode=UI_MAIN_settings.reducedMode;
                    return 1;
                    break;

        case UI_MAIN_SETUP_MENU_SM_REDUCED_MODE:
                    SCREENS_reducedModeSetup(UI_MAIN_settings.reducedMode,1);
                    while(UI_MAIN_readKeyAndVLCQueue(1,10,&rxData)!=0){//wait for incoming key messages
                        if(reducedMode) setupMenuReducedModeLastTimestamp=now;
                        if((rxData.input_message.key==INPUT_KEY_DOWN)||(rxData.input_message.key==INPUT_KEY_DOWN_STRONG)){
                            if(UI_MAIN_settings.reducedMode==0){
                                UI_MAIN_settings.reducedMode=1;
                            }else{
                                UI_MAIN_settings.reducedMode=0;
                            }
                        }else if((rxData.input_message.key==INPUT_KEY_UP)||(rxData.input_message.key==INPUT_KEY_UP_STRONG)){
                            if(UI_MAIN_settings.reducedMode==1){
                                UI_MAIN_settings.reducedMode=0;
                            }else{
                                UI_MAIN_settings.reducedMode=1;
                            }
                        }else if(rxData.input_message.key==INPUT_KEY_OK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }else if(rxData.input_message.key==INPUT_KEY_BACK){
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    if(reducedMode){
                        if((now-setupMenuReducedModeLastTimestamp)/1000>10000){//go back after 10s no input in reduced mode
                            setupMenuReducedModeLastTimestamp=now;
                            UI_MAIN_setupMenuData.sm=UI_MAIN_SETUP_MENU_SUB_SELECTION;
                        }
                    }
                    break;

    }
    return 0;
}
#endif
