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
#ifndef UTIL_C
#define UTIL_C
#include"util.h"
#include<time.h>
#include <stdio.h>
#include <string.h>


uint64_t UTIL_get_time_us(){
    struct timespec current;
    clock_gettime( CLOCK_REALTIME, &current);
    return current.tv_sec*1000000+current.tv_nsec/1000;
}

int UTIL_utf8_char_length(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    else if ((c & 0xE0) == 0xC0) return 2;
    else if ((c & 0xF0) == 0xE0) return 3;
    else if ((c & 0xF8) == 0xF0) return 4;
    else return 1; // invalid UTF-8 start byte
}

int32_t UTIL_utf8_char_at(const char *str, int32_t char_index, char *out_char) {
    const unsigned char *p = (const unsigned char *)str;
    int32_t i = 0;

    while (*p) {
        int32_t len = UTIL_utf8_char_length(*p);
        if (i == char_index) {
            for (int32_t j = 0; j < len; j++) {
                out_char[j] = p[j];
            }
            out_char[len] = '\0'; // null-terminate
            return len;
        }
        p += len;
        i++;
    }

    return 0; // Index out of bounds
}

void UTIL_getCPUGovernor(char *governor){
    FILE *bl;
    governor[0]=0;
    bl=fopen("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor","r");
    if(bl){
        fgets(governor,250,bl);
        if(governor[strlen(governor)-1]==10){
            governor[strlen(governor)-1]=0;
        }
        fclose(bl);
    }
}

void UTIL_setCPUGovernor(char *governor){
    FILE *bl;
    bl=fopen("/sys/devices/system/cpu/cpufreq/policy0/scaling_governor","r+");
    if(bl){
        fprintf(bl, "%s\n",governor);
        fclose(bl);
    }
}


#endif
