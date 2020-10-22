/*
 * sweeprom.cpp
 *
 * just the functions
 */

#include "sweeprom.h"

SWEeprom initSwEEPROM()
{
    SWEeprom ret;

    ret.version = EEPROMVERSION;
    ret.status[0]  = 50;
    ret.status[1]  = 1;
    ret.status[2]  = 1;
    ret.status[3]  = 1;
    writeSwEEPROM(ret);
    
    return ret;
}

SWEeprom readSwEEPROM()
{
    SWEeprom ret;
    int i;

    ret.version   = EEPROM.read(0);
    for (i=0; i<5 ; i++) {
        ret.status[i] = ( (EEPROM.read(1+4*i)*256+EEPROM.read(2+4*i))*256 + EEPROM.read(3+4*i))*256 + EEPROM.read(4+4*i);
    }
    
    return ret;
}

/* only write changed data */
void writeSwEEPROM(SWEeprom newdata)
{
    SWEeprom olddata = readSwEEPROM();
    int i;
    long value;

    EEPROM.begin(sizeof(SWEeprom));
    for (i=0; i<5 ; i++) {
        if (olddata.status[i] != newdata.status[i]){
            value = newdata.status[i];
            EEPROM.write(4+4*i, value % 256);
            value = value / 256;
            EEPROM.write(3+4*i, value % 256);
            value = value / 256;
            EEPROM.write(2+4*i, value % 256);
            EEPROM.write(1+4*i, value / 256);
        }
    }
    EEPROM.commit();
}
