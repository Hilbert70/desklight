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

void handleStart(long *start)
{
    if (*start > MAXBAR) *start = MAXBAR;
    if (*start < 0) *start = 0;
}
void handleLength(long *length)
{
    if (*length > MAXBAR) *length = MAXBAR;
    if (*length <1 ) *length = 1;
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
void limitStart(long *start, long length)
{
    if (*start > MAXBAR-length)  *start = MAXBAR-length;
    if (*start + length > MAXBAR)  *start =MAXBAR - length;
}
