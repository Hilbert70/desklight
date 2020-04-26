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
    ret.delay   = 8;
    ret.voltageWarning = 1140;   // 3*3.8 low voltage alarm
    ret.voltageLow     = 1080;   // 3*3.6 low volgage

    writeSwEEPROM(ret);
    
    return ret;
}

SWEeprom readSwEEPROM()
{
    SWEeprom ret;

    ret.version = EEPROM.read(0);
    ret.delay   = EEPROM.read(1)*256+EEPROM.read(2);
    ret.voltageWarning = EEPROM.read(3)*256+EEPROM.read(4);
    ret.voltageLow     = EEPROM.read(5)*256+EEPROM.read(6);
    
    return ret;
}

/* only write changed data */
void writeSwEEPROM(SWEeprom newdata)
{
    SWEeprom olddata = readSwEEPROM();

    if (olddata.delay != newdata.delay){
	EEPROM.write(1, newdata.delay / 256);
	EEPROM.write(2, newdata.delay % 256);
    }

    if (olddata.voltageWarning != newdata.voltageWarning){
	EEPROM.write(1, newdata.voltageWarning / 256);
	EEPROM.write(2, newdata.voltageWarning % 256);
    }
    if (olddata.voltageLow != newdata.voltageLow){
	EEPROM.write(1, newdata.voltageLow / 256);
	EEPROM.write(2, newdata.voltageLow % 256);
    }
}
