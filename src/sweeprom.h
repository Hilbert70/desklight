#ifndef __SWEEPROM_H
#define __SWEEPROM_H

/*
 * sweerpom.h
 *
 * structure of the eeporm and support functions
 */
#include <EEPROM.h>

#define EEPROMVERSION 1

#define ST_DIM 0
#define ST_START 1
#define ST_LENGTH 2
#define ST_LEDS 3


typedef struct {
    char version;
    long  status[4];
} SWEeprom;
    
extern SWEeprom initSwEEPROM();
extern SWEeprom readSwEEPROM();
extern void writeSwEEPROM(SWEeprom);

#endif
