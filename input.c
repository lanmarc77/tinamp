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
#ifndef INPUT_C
#define INPUT_C
#include <SDL2/SDL.h>
#include"input.h"
#include"util.h"

Queue *INPUT_outQueue;
SDL_Thread *INPUT_eventLoopThread;
uint8_t INPUT_abSwitch=0;
uint64_t INPUT_keyTimeouts[INPUT_KEY_LONG_ADDITION]={0};

void INPUT_switchAB(uint8_t abSwitch){
    INPUT_abSwitch=abSwitch;
    for(int i=0;i<INPUT_KEY_LONG_ADDITION;i++){
        INPUT_keyTimeouts[i]=0;
    }
}

int INPUT_SDL_EventLoop(void *user_data) {
    uint64_t keyState=0;
    SDL_Event e;
    uint8_t quit=0;

    QueueData data;
    while(!quit){
        data.input_message.key=INPUT_KEY_NONE;
        for(int i=0;i<INPUT_KEY_LONG_ADDITION;i++){
            if((INPUT_keyTimeouts[i]!=0)&&(INPUT_keyTimeouts[i]!=1)){
                if((UTIL_get_time_us()-INPUT_keyTimeouts[i])>3000000){
                    INPUT_keyTimeouts[i]=1;
                    data.input_message.key=i+INPUT_KEY_LONG_ADDITION;
                }
            }
        }
        if( SDL_WaitEventTimeout( &e ,1000) != 0 ) {
            switch (e.type) {
                case SDL_QUIT:
                    data.input_message.key=INPUT_KEY_SELECT_AND_START;
                    quit=1;
                    break;
                case SDL_CONTROLLERBUTTONUP:
                    if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP){//dpad up
                        keyState&=~(1<<INPUT_KEY_UP_STRONG);
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN){//dpad down
                        keyState&=~(1<<INPUT_KEY_DOWN_STRONG);
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT){//dpad right
                        keyState&=~(1<<INPUT_KEY_UP);
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT){//dpad left
                        keyState&=~(1<<INPUT_KEY_DOWN);
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_START){//start
                        keyState&=~(1<<INPUT_KEY_START);
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_BACK){//select
                        keyState&=~(1<<INPUT_KEY_SELECT);
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_A){//A
                        if(INPUT_abSwitch==0){
                            keyState&=~(1<<INPUT_KEY_OK);
                        }else{
                            if(INPUT_keyTimeouts[INPUT_KEY_BACK]>10){
                                data.input_message.key=INPUT_KEY_BACK;
                                INPUT_keyTimeouts[INPUT_KEY_BACK]=0;
                            }
                            keyState&=~(1<<INPUT_KEY_BACK);
                        }
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_B){//B
                        if(INPUT_abSwitch==0){
                            if(INPUT_keyTimeouts[INPUT_KEY_BACK]>10){
                                data.input_message.key=INPUT_KEY_BACK;
                                INPUT_keyTimeouts[INPUT_KEY_BACK]=0;
                            }
                            keyState&=~(1<<INPUT_KEY_BACK);
                        }else{
                            keyState&=~(1<<INPUT_KEY_OK);
                        }
                    }
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    if(e.cbutton.state!=SDL_PRESSED) break;//no repeats for buttons
                    if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP){//dpad up
                        data.input_message.key=INPUT_KEY_UP_STRONG;
                        keyState|=1<<INPUT_KEY_UP_STRONG;
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN){//dpad down
                        data.input_message.key=INPUT_KEY_DOWN_STRONG;
                        keyState|=1<<INPUT_KEY_DOWN_STRONG;
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT){//dpad right
                        data.input_message.key=INPUT_KEY_UP;
                        keyState|=1<<INPUT_KEY_UP;
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT){//dpad left
                        data.input_message.key=INPUT_KEY_DOWN;
                        keyState|=1<<INPUT_KEY_DOWN;
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_START){//start
                        if(keyState&(1<<INPUT_KEY_SELECT)){
                            data.input_message.key=INPUT_KEY_SELECT_AND_START;
                        }else{
                            data.input_message.key=INPUT_KEY_START;
                        }
                        keyState|=1<<INPUT_KEY_START;
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_BACK){//select
                        if(keyState&(1<<INPUT_KEY_START)){
                            data.input_message.key=INPUT_KEY_SELECT_AND_START;
                        }else{
                            data.input_message.key=INPUT_KEY_SELECT;
                        }
                        keyState|=1<<INPUT_KEY_SELECT;
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_A){//A
                        if(INPUT_abSwitch==0){
                            data.input_message.key=INPUT_KEY_OK;
                            keyState|=1<<INPUT_KEY_OK;
                        }else{
                            INPUT_keyTimeouts[INPUT_KEY_BACK]=UTIL_get_time_us();
                            keyState|=1<<INPUT_KEY_BACK;
                        }
                    }else if(e.cbutton.button==SDL_CONTROLLER_BUTTON_B){//B
                        if(INPUT_abSwitch==0){
                            INPUT_keyTimeouts[INPUT_KEY_BACK]=UTIL_get_time_us();
                            keyState|=1<<INPUT_KEY_BACK;
                        }else{
                            data.input_message.key=INPUT_KEY_OK;
                            keyState|=1<<INPUT_KEY_OK;
                        }
                    }else{
                        data.input_message.key=INPUT_KEY_OTHER;
                    }
                    break;
                case SDL_KEYUP:
                    if(e.key.repeat==0){//make keyboard behave like controller without repeat for local development
                        if(e.key.keysym.scancode==82){//up
                            keyState&=~(1<<INPUT_KEY_UP_STRONG);
                        }else if(e.key.keysym.scancode==81){//down
                            keyState&=~(1<<INPUT_KEY_DOWN_STRONG);
                        }else if(e.key.keysym.scancode==79){//right
                            keyState&=~(1<<INPUT_KEY_UP);
                        }else if(e.key.keysym.scancode==80){//left
                            keyState&=~(1<<INPUT_KEY_DOWN);
                        }else if(e.key.keysym.scancode==40){//enter=start
                            keyState&=~(1<<INPUT_KEY_START);
                        }else if(e.key.keysym.scancode==44){//space=select
                            keyState&=~(1<<INPUT_KEY_SELECT);
                        }else if(e.key.keysym.scancode==4){//a=ok
                            if(INPUT_abSwitch==0){
                                keyState&=~(1<<INPUT_KEY_OK);
                            }else{
                                if(INPUT_keyTimeouts[INPUT_KEY_BACK]>10){
                                    data.input_message.key=INPUT_KEY_BACK;
                                    INPUT_keyTimeouts[INPUT_KEY_BACK]=0;
                                }
                                keyState&=~(1<<INPUT_KEY_BACK);
                            }
                        }else if(e.key.keysym.scancode==5){//b=back
                            if(INPUT_abSwitch==0){
                                if(INPUT_keyTimeouts[INPUT_KEY_BACK]>10){
                                    data.input_message.key=INPUT_KEY_BACK;
                                    INPUT_keyTimeouts[INPUT_KEY_BACK]=0;
                                }
                                keyState&=~(1<<INPUT_KEY_BACK);
                            }else{
                                keyState&=~(1<<INPUT_KEY_OK);
                            }
                        }
                    }
                    break;
                case SDL_KEYDOWN:
                    if(e.key.repeat==0){//make keyboard behave like controller without repeat for local development
                        if(e.key.keysym.scancode==82){//up
                            data.input_message.key=INPUT_KEY_UP_STRONG;
                            keyState|=1<<INPUT_KEY_UP_STRONG;
                        }else if(e.key.keysym.scancode==81){//down
                            data.input_message.key=INPUT_KEY_DOWN_STRONG;
                            keyState|=1<<INPUT_KEY_DOWN_STRONG;
                        }else if(e.key.keysym.scancode==79){//right
                            data.input_message.key=INPUT_KEY_UP;
                            keyState|=1<<INPUT_KEY_UP;
                        }else if(e.key.keysym.scancode==80){//left
                            data.input_message.key=INPUT_KEY_DOWN;
                            keyState|=1<<INPUT_KEY_DOWN;
                        }else if(e.key.keysym.scancode==40){//enter=start
                            if(keyState&(1<<INPUT_KEY_SELECT)){
                                data.input_message.key=INPUT_KEY_SELECT_AND_START;
                            }else{
                                data.input_message.key=INPUT_KEY_START;
                            }
                            keyState|=1<<INPUT_KEY_START;
                        }else if(e.key.keysym.scancode==44){//space=select
                            if(keyState&(1<<INPUT_KEY_START)){
                                data.input_message.key=INPUT_KEY_SELECT_AND_START;
                            }else{
                                data.input_message.key=INPUT_KEY_SELECT;
                            }
                            keyState|=1<<INPUT_KEY_SELECT;
                        }else if(e.key.keysym.scancode==4){//a=ok
                            if(INPUT_abSwitch==0){
                                data.input_message.key=INPUT_KEY_OK;
                                keyState|=1<<INPUT_KEY_OK;
                            }else{
                                INPUT_keyTimeouts[INPUT_KEY_BACK]=UTIL_get_time_us();
                                keyState|=1<<INPUT_KEY_BACK;
                            }
                        }else if(e.key.keysym.scancode==5){//b=back
                            if(INPUT_abSwitch==0){
                                INPUT_keyTimeouts[INPUT_KEY_BACK]=UTIL_get_time_us();
                                keyState|=1<<INPUT_KEY_BACK;
                            }else{
                                data.input_message.key=INPUT_KEY_OK;
                                keyState|=1<<INPUT_KEY_OK;
                            }
                        }else{
                            data.input_message.key=INPUT_KEY_OTHER;
                        }
                    }
                    break;
                case SDL_WINDOWEVENT:
                        if(e.window.event==SDL_WINDOWEVENT_RESIZED){
                            data.input_message.key=INPUT_WINDOW_RESIZE;
                        }
                        break;
                case SDL_CONTROLLERDEVICEADDED:
                    //if (!INPUT_controller) {
                        //INPUT_controller = SDL_GameControllerOpen(e.cdevice.which);
                        //SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Added controller with mapping %s\n",SDL_GameControllerMapping(INPUT_controller));
                    //}
                    break;
                /*case SDL_CONTROLLERDEVICEREMOVED:
                    if (controller && event.cdevice.which == SDL_JoystickInstanceID(
                        SDL_GameControllerGetJoystick(controller))) {
                        SDL_GameControllerClose(controller);
                        controller = findController();
                    }
                    return false;*/
            }
        }
        if(data.input_message.key!=INPUT_KEY_NONE){
            data.type=QUEUE_DATA_INPUT;
            queue_push(INPUT_outQueue,&data);
        }
    }
    return 0;
}


void INPUT_init(Queue *outputQueue){
    SDL_GameController *controller = NULL;
    if (getenv("SDL_GAMECONTROLLERCONFIG") != NULL){
        SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Using environment variable for game controller db file: %s\n",getenv("SDL_GAMECONTROLLERCONFIG"));
        if(SDL_GameControllerAddMappingsFromFile(getenv("SDL_GAMECONTROLLERCONFIG"))<0){
            SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_WARN,"Could not get SDL game controller mapping db file: %s\n",SDL_GetError());
        }
    }else if (getenv("SDL_GAMECONTROLLERCONFIG_FILE") != NULL){
        SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Using environment variable for game controller db file: %s\n",getenv("SDL_GAMECONTROLLERCONFIG_FILE"));
        if(SDL_GameControllerAddMappingsFromFile(getenv("SDL_GAMECONTROLLERCONFIG_FILE"))<0){
            SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_WARN,"Could not get SDL game controller mapping db file: %s\n",SDL_GetError());
        }
    }else{//if all else fails try hardcoded set of possible locations
        if(SDL_GameControllerAddMappingsFromFile("/storage/.config/SDL-GameControllerDB/gamecontrollerdb.txt")>=0){//rocknix
            SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Found usable SDL game controller mapping db file\n");
        }else if(SDL_GameControllerAddMappingsFromFile("/opt/muos/device/current/control/gamecontrollerdb_retro.txt")>=0){//muos
            SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Found usable SDL game controller mapping db file\n");
        }else{
            SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_WARN,"No SDL game controller mapping db file could be used. Try setting environemt variable SDL_GAMECONTROLLERCONFIG.\n");
        }
    }
    /* Print information about the controller */
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Found Joystick %i\n",i);
        if (SDL_IsGameController(i)) {
            SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Found GameController %i\n",i);
            controller = SDL_GameControllerOpen(i);
            SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Added controller with mapping %s\n",SDL_GameControllerMapping(controller));
        }
    }

    INPUT_outQueue=outputQueue;
    INPUT_eventLoopThread=SDL_CreateThread(INPUT_SDL_EventLoop, "input_sdl_events",NULL);
    SDL_LogMessage(SDL_LOG_CATEGORY_INPUT,SDL_LOG_PRIORITY_INFO,"Initialized SDL input.\n");
}

void INPUT_close(){
    SDL_Event e;
    int state;
    e.type=SDL_QUIT;
    SDL_PushEvent(&e);
    SDL_WaitThread(INPUT_eventLoopThread,&state);
}



#endif
