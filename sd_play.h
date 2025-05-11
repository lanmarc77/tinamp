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
#ifndef SD_PLAY_H
#define SD_PLAY_H
#include <stdint.h>
#include "queue.h"

//all msgTypes
#define SD_PLAY_MSG_TYPE_START_PLAY 1
#define SD_PLAY_MSG_TYPE_STOP_PLAY 2
#define SD_PLAY_MSG_TYPE_STOPPED_NO_ERROR 3
#define SD_PLAY_MSG_TYPE_STOPPED_ERROR 4
#define SD_PLAY_MSG_TYPE_FILEPOS_STATE 5
#define SD_PLAY_MSG_TYPE_PAUSE  6
#define SD_PLAY_MSG_TYPE_PAUSED 7
#define SD_PLAY_MSG_TYPE_RESUME  8
#define SD_PLAY_MSG_TYPE_RESUMED  9

void SD_PLAY_init(Queue *out);
void SD_PLAY_close();
uint8_t SD_PLAY_startService();
void SD_PLAY_sendMessage(QueueData* msg);
uint8_t SD_PLAY_getMessage(SD_PLAY_message_t* msg, uint16_t waitTime);

void SD_PLAY_setVolume(int64_t volume);
void SD_PLAY_setPlaySpeed(uint16_t playSpeed);
void SD_PLAY_setEqualizer(uint8_t equalizer);
Queue* SD_PLAY_getMessageQueue();
int64_t SD_PLAY_getRuntime(char *file);

#endif
