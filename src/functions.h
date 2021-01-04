#ifndef __FUNCTIONS_H
#define __FUNCTIONS_H

#include <arduino.h>

// number of elements in the bar, for testing just 3
#define MAXBAR 16
#define MAXMENU 4
#define MAXMODES 6
#define MAXLIGHT 4095

long StrtoLong(String str);
void handleStart(long *start);
void handleLength(long *length);
void handleBrightness(long *brightness);
void handleColour(long *colour);
void limitStart(long *start, long length);

#endif