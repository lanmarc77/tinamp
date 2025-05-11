#ifndef UTIL_H
#define UTIL_H
#include<stdint.h>

uint64_t UTIL_get_time_us();
#define UI_MAIN_REPEAT_FOLDER 1
int32_t UTIL_utf8_char_at(const char *str, int32_t char_index, char *out_char);
void UTIL_getCPUGovernor(char *governor);
void UTIL_setCPUGovernor(char *governor);


#endif
