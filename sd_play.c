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
#ifndef SD_PLAY_C
#define SD_PLAY_C
#include <SDL2/SDL.h>
#include <string.h>
#include <sys/stat.h>
#include <vlc/vlc.h>
#include "sd_play.h"
#include "format_helper.h"
#include "globals.h"

Queue SD_PLAY_inQueue;
Queue* SD_PLAY_outQueue=NULL;
libvlc_instance_t* SD_PLAY_vlcInst=NULL;
SDL_Thread *SD_PLAY_messageLoop=NULL;
SDL_Thread *SD_PLAY_playLoop=NULL;
libvlc_media_t* SD_PLAY_currentMedia=NULL;
libvlc_media_player_t* SD_PLAY_currentPlayer=NULL;
int64_t SD_PLAY_currentVolume = 0;

void SD_PLAY_setVolume(int64_t volume) {
    SD_PLAY_currentVolume=volume;
    if(SD_PLAY_currentPlayer!=NULL){
        libvlc_audio_set_volume(SD_PLAY_currentPlayer, SD_PLAY_currentVolume);
    }
}

/**
  * @brief get the queue handle of the sd play output channel
  *        this is needed to allow putting this in a queue set
  *
  * @return queue handle
  * 
  */
Queue *SD_PLAY_getMessageQueue(){
    return SD_PLAY_outQueue;
}

uint8_t SD_PLAY_currentEqualizer = 0;
libvlc_equalizer_t* SD_PLAY_currentEqualizerProfile = NULL;
uint8_t SD_PLAY_equalizerMapping[5]={0,4,5,6,7};


/**
  * @brief sets a new equalizer preset
  *
  * @param equalizer new preset, 0=neutral
  * 
  */
void SD_PLAY_setEqualizer(uint8_t equalizer){
    if(SD_PLAY_currentEqualizerProfile!=NULL){
        if(SD_PLAY_currentPlayer!=NULL){
            libvlc_media_player_set_equalizer(SD_PLAY_currentPlayer, NULL);
            libvlc_audio_equalizer_release(SD_PLAY_currentEqualizerProfile);
            SD_PLAY_currentEqualizerProfile=NULL;
        }
    }
    SD_PLAY_currentEqualizer=equalizer;
    if(equalizer!=0){
        if(SD_PLAY_currentPlayer!=NULL){
            SD_PLAY_currentEqualizerProfile = libvlc_audio_equalizer_new_from_preset(SD_PLAY_equalizerMapping[SD_PLAY_currentEqualizer]);
            libvlc_media_player_set_equalizer(SD_PLAY_currentPlayer, SD_PLAY_currentEqualizerProfile);
        }
    }
}

float SD_PLAY_currentPlaySpeed = 1.00;

/**
  * @brief sets a new play speed, works during playing and in paused/stopped mode
  *
  * @param volume new play speed 50...300 equals x0.50...x3.00
  * 
  */
void SD_PLAY_setPlaySpeed(uint16_t playSpeed){
    if(playSpeed>300){
        playSpeed=300;
    }
    if(playSpeed<50){
        playSpeed=50;
    }
    SD_PLAY_currentPlaySpeed=playSpeed/100.0;
    if(SD_PLAY_currentPlayer!=NULL){
        libvlc_media_player_set_rate(SD_PLAY_currentPlayer, SD_PLAY_currentPlaySpeed);
    }
}

uint8_t SD_PLAY_startPlaying(char* file,uint64_t filePos){
    if(SD_PLAY_currentPlayer!=NULL){
        libvlc_media_player_stop(SD_PLAY_currentPlayer);
        libvlc_media_player_release(SD_PLAY_currentPlayer);
        SD_PLAY_currentPlayer=NULL;
    }
    if(SD_PLAY_currentMedia!=NULL){
        libvlc_media_release(SD_PLAY_currentMedia);
        SD_PLAY_currentMedia=NULL;
    }
    SD_PLAY_currentMedia = libvlc_media_new_path (SD_PLAY_vlcInst, file);
    SD_PLAY_currentPlayer = libvlc_media_player_new_from_media (SD_PLAY_currentMedia);
    while(libvlc_audio_set_volume(SD_PLAY_currentPlayer, SD_PLAY_currentVolume)!=0){
        SDL_Delay(10);
    }
    while(libvlc_media_player_set_rate(SD_PLAY_currentPlayer, SD_PLAY_currentPlaySpeed)!=0){
        SDL_Delay(10);
    }
    libvlc_media_player_play (SD_PLAY_currentPlayer);
    while(libvlc_media_player_get_state(SD_PLAY_currentPlayer)!=libvlc_Playing){
        SDL_Delay(10);
    }
    libvlc_media_player_set_time(SD_PLAY_currentPlayer,filePos);
    SD_PLAY_setEqualizer(SD_PLAY_currentEqualizer);
    return 0;

}
int SD_PLAY_messageLoopThread(void *user_data){
    uint64_t a_milliseconds=0;
    QueueData data;
    QueueData* dataPtr;
    dataPtr=&data;
    uint64_t lastMillisecond=-1;
    uint8_t amrFix=0;
    while(1){
	dataPtr=&data;
        queue_pop_timeout(&SD_PLAY_inQueue,&dataPtr,100);
        if(dataPtr!=NULL){
            if(data.type==QUEUE_DATA_SD_PLAY){
                //SDL_LogMessage(SDL_LOG_CATEGORY_AUDIO,SDL_LOG_PRIORITY_INFO,"Message in AUDIO received. %i\n",data.sd_play_message.msgType);
                if(data.sd_play_message.msgType==SD_PLAY_MSG_TYPE_START_PLAY){
                    amrFix=0;
                    data.sd_play_message.msgType=SD_PLAY_MSG_TYPE_START_PLAY;
#ifdef FFMPEG_AMR_PATCH
                    if(FORMAT_HELPER_getFileType(data.sd_play_message.msgData)==FORMAT_HELPER_FILE_TYPE_AMR){
                        SDL_LogMessage(SDL_LOG_CATEGORY_AUDIO,SDL_LOG_PRIORITY_INFO,"AMR detected. Bugfix for https://trac.ffmpeg.org/ticket/11576.\n");
                        amrFix=1;
                    }
#endif
                    queue_push(SD_PLAY_outQueue,&data);
                    SD_PLAY_startPlaying(data.sd_play_message.msgData,data.sd_play_message.startPosition);
                    lastMillisecond=-1;
                }else if(data.sd_play_message.msgType==SD_PLAY_MSG_TYPE_PAUSE){
                    libvlc_media_player_set_pause(SD_PLAY_currentPlayer, 1);
                    data.sd_play_message.msgType=SD_PLAY_MSG_TYPE_PAUSED;
                    queue_push(SD_PLAY_outQueue,&data);
                }else if(data.sd_play_message.msgType==SD_PLAY_MSG_TYPE_RESUME){
                    libvlc_media_player_set_pause(SD_PLAY_currentPlayer, 0);
                    data.sd_play_message.msgType=SD_PLAY_MSG_TYPE_RESUMED;
                    queue_push(SD_PLAY_outQueue,&data);
                }
            }else if(data.type==QUEUE_DATA_QUIT){
                if(SD_PLAY_currentPlayer!=NULL){
                    libvlc_media_player_stop(SD_PLAY_currentPlayer);
                }
                return 0;
            }
        }
        if(SD_PLAY_currentPlayer!=NULL){
            libvlc_state_t vlc_state=libvlc_media_player_get_state(SD_PLAY_currentPlayer);
            if (vlc_state==libvlc_Playing)
            {
                if(libvlc_media_player_get_length(SD_PLAY_currentPlayer)>0){
                    a_milliseconds = libvlc_media_player_get_length(SD_PLAY_currentPlayer);
                    if(amrFix) a_milliseconds=(a_milliseconds*100000000)/103333333;

                }
                int64_t c_milliseconds = libvlc_media_player_get_time(SD_PLAY_currentPlayer);
                if(c_milliseconds/1000!=lastMillisecond){//only send updates if a full second changes, saves CPU time in main
                    lastMillisecond=c_milliseconds/1000;
                    data.type=QUEUE_DATA_SD_PLAY;
                    data.sd_play_message.msgType=SD_PLAY_MSG_TYPE_FILEPOS_STATE;
                    data.sd_play_message.currentPosition=c_milliseconds;
                    data.sd_play_message.trackLength=a_milliseconds;
                    queue_push(SD_PLAY_outQueue,&data);
                }
            }else if (vlc_state==libvlc_Ended){
                data.type=QUEUE_DATA_SD_PLAY;
                data.sd_play_message.msgType=SD_PLAY_MSG_TYPE_STOPPED_NO_ERROR;
                queue_push(SD_PLAY_outQueue,&data);
            }else if (vlc_state==libvlc_Error){
                data.type=QUEUE_DATA_SD_PLAY;
                data.sd_play_message.msgType=SD_PLAY_MSG_TYPE_STOPPED_ERROR;
                queue_push(SD_PLAY_outQueue,&data);
            }
        }
    }
    return 0;
}

void SD_PLAY_init(Queue *out){
    SD_PLAY_outQueue=out;
    queue_init(&SD_PLAY_inQueue);
    const char *vlcArgs[] = {
        "--no-xlib",
        "--verbose=2"
    };
    SD_PLAY_vlcInst = libvlc_new (2, vlcArgs);
    if(SD_PLAY_vlcInst==NULL){
        SDL_LogMessage(SDL_LOG_CATEGORY_AUDIO,SDL_LOG_PRIORITY_ERROR,"VLC could not initialized.\n");
    }else{
        SDL_LogMessage(SDL_LOG_CATEGORY_AUDIO,SDL_LOG_PRIORITY_INFO,"VLC initialized.\n");
    }
    SD_PLAY_messageLoop=SDL_CreateThread(SD_PLAY_messageLoopThread, "sd_play_message_loop",NULL);

    /*printf("Bands...\n");
    unsigned u_bands = libvlc_audio_equalizer_get_band_count();
    for(int i = 0; i < u_bands; i++) {
        printf(" Band %2d -> %.1fHz\n", i, libvlc_audio_equalizer_get_band_frequency(i));
    }
    printf("\n");
    printf("Presets...\n");
    unsigned u_presets = libvlc_audio_equalizer_get_preset_count();
    for(int i = 0; i < u_presets; i++) {
        printf(" Preset %2d -> %s\n", i, libvlc_audio_equalizer_get_preset_name(i));
    }
    printf("\n");*/
}


void SD_PLAY_close(){
    QueueData data;
    data.type=QUEUE_DATA_QUIT;
    SD_PLAY_sendMessage(&data);
    int state=0;
    SDL_WaitThread(SD_PLAY_messageLoop,&state);
}


void SD_PLAY_sendMessage(QueueData* msg){
    queue_push(&SD_PLAY_inQueue,msg);
}

/**
  * @brief received a message from the sd play service
  *
  * @param msg pointer to the message to get
  * @param waitTime wait time in ms to wait for message queue reading
  * 
  * @return 0=ok message received, 2=timeout no new message available
  * 
  */
uint8_t SD_PLAY_getMessage(SD_PLAY_message_t* msg, uint16_t waitTime){
/*    if(SD_PLAY_outQueue==NULL) return 1;
    memset(msg,0,sizeof(SD_PLAY_message_t));
    if(xQueueReceive(SD_PLAY_outQueue,msg,pdMS_TO_TICKS(waitTime)) != pdPASS ){
        return 2;
    }*/
    return 0;
}

int64_t SD_PLAY_getRuntime(char *file){
    int64_t r;
    if(SD_PLAY_vlcInst!=NULL){
        libvlc_media_t* m=libvlc_media_new_path(SD_PLAY_vlcInst,file);
        if(m!=NULL){
            if(libvlc_media_parse_with_options(m,libvlc_media_parse_local,10000)<0){
                libvlc_media_release(m);
                return -2;
            }
            while(libvlc_media_get_parsed_status(m)!=libvlc_media_parsed_status_done){
                //TODO: implement timeout and quit with error
                SDL_Delay(10);
            }
            r=libvlc_media_get_duration(m);
#ifdef FFMPEG_AMR_PATCH
            if(FORMAT_HELPER_getFileType(file)==FORMAT_HELPER_FILE_TYPE_AMR){
                SDL_LogMessage(SDL_LOG_CATEGORY_AUDIO,SDL_LOG_PRIORITY_INFO,"AMR detected. Bugfix for https://trac.ffmpeg.org/ticket/11576. Original runtime %li.\n",r);
                r=(r*100000000)/103333333;
                SDL_LogMessage(SDL_LOG_CATEGORY_AUDIO,SDL_LOG_PRIORITY_INFO,"AMR detected. Bugfix for https://trac.ffmpeg.org/ticket/11576. Adjusted runtime %li.\n",r);
            }
#endif
            libvlc_media_release(m);
            return r;
        }
    }
    SDL_LogMessage(SDL_LOG_CATEGORY_AUDIO,SDL_LOG_PRIORITY_ERROR,"VLC instance NULL %s.\n",file);
    return -2;
}


#endif
