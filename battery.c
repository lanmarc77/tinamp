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
#ifndef BATTERY_C
#define BATTERY_C
#include<stdio.h>
#include<stdlib.h>
#include"battery.h"


uint8_t BATTERY_getPercent(){
    uint8_t p=0;
    FILE *bl;
    char line[100]={};
    bl=fopen("/sys/class/power_supply/battery/capacity","r");
    if(bl){
	if(fgets(&line[0],sizeof(line),bl)!= NULL){
	    p=atol(&line[0]);
	}
	fclose(bl);
    }
    return p;
}

#endif
