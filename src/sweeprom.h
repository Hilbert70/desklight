#ifndef __SWEEPROM_H
#define __SWEEPROM_H

/*
 * sweerpom.h
 *
 * structure of the eeporm and support functions
 */
#include <EEPROM.h>

#define EEPROMVERSION 0

typedef struct {
    char version;
    int  delay;
    int  voltageWarning;
    int  voltageLow;
} SWEeprom;
    
extern SWEeprom initSwEEPROM();
extern SWEeprom readSwEEPROM();
extern void writeSwEEPROM(SWEeprom);

#endif
