#include "functions.h"


long StrtoLong(String str){
    long value=0;
    int i,len;
    
    len = str.length();
    for (i=0; i<len;i++){
        if (isDigit(str[i])) {
            value = value*10 + str[i] - '0';
        } else {
            break;
        }
    }
    return value;
}

void handleStart(long *start, long length)
{
    if (*start > MAXBAR) *start = MAXBAR;
    if (*start < 0) *start = 0;
    if (*start > MAXBAR-length)  *start = MAXBAR-length;
}
void handleLength(long *start,long *length, long increment)
{
    if (*length <1 ) *length = 1;
    if (*length > MAXBAR) {
        *length = MAXBAR;
        *start  = 0;
    }
    if (*length % 2 == 1){
        *start = *start - increment;
    }
    if ((*start) < 0) {
        *start  = 0;
         *length = *length + 1;
        if (*length > MAXBAR) {
            *length = MAXBAR;
        }
    }
    if (*start + *length > MAXBAR)  *start =MAXBAR - *length;
}
void handleBrightness(long *brightness)
{
    if (*brightness < 1 ) *brightness = 1;
    if (*brightness > MAXLIGHT ) *brightness = MAXLIGHT;
}
void handleColour(long *colour)
{
    if (*colour < 0 ) *colour = 0;
    if (*colour > MAXMODES ) *colour = MAXMODES-1;
}
/*
void limitStart(long *start, long *length, boolean doingStart)
{
    if (!doingStart) {
        if ((*length) % 2 ==1 ) {
            (*start)--;
        }
        if (*start < 0) {
            *start = 0;
            (*length)++;
            if (*length > MAXBAR) *length = MAXBAR;
        }
    }
    if (*start > MAXBAR-*length)  *start = MAXBAR-*length;
    if (*start + *length > MAXBAR)  *start =MAXBAR - *length;
}
*/