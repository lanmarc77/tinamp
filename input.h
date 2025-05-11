#ifndef INPUT_H
#define INPUT_H
#include<stdint.h>
#include "queue.h"

#define INPUT_KEY_NONE 0
#define INPUT_KEY_UP 1
#define INPUT_KEY_DOWN 2
#define INPUT_KEY_UP_STRONG 3
#define INPUT_KEY_DOWN_STRONG 4
#define INPUT_KEY_START 5
#define INPUT_KEY_SELECT 6
#define INPUT_KEY_SELECT_LONG 7
#define INPUT_KEY_OK 8
#define INPUT_KEY_BACK 9
#define INPUT_KEY_OTHER 99

#define INPUT_WINDOW_RESIZE 100

void INPUT_init(Queue *outputQueue);
void INPUT_close();
void INPUT_switchAB(uint8_t abSwitch);

#endif
