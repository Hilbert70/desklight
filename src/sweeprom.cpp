/*
 * sweeprom.cpp
 *
 * just the functions
 */

#include "sweeprom.h"


long readLong(int addres){
    return ( (EEPROM.read(addres)*256+EEPROM.read(1+addres))*256 + EEPROM.read(2+addres))*256 + EEPROM.read(3+addres);
}

SWEeprom::SWEeprom()
{
    Ewritten = false;
    EEPROM.begin(512);
}

long * SWEeprom::init()
{
    char oldVersion;

    oldVersion = EEPROM.read(0);

    if (oldVersion != EEPROMVERSION) {
        // 'upgrade'
        version = EEPROMVERSION;
        status[0]  = 50;
        status[1]  = 4;
        status[2]  = 3;
        status[3]  = 2;
        write();
    } else {
        read();
    }

    return status;
}

long * SWEeprom::read()
{
    int i;
    version   = EEPROM.read(0);

    for (i=0; i<4 ; i++) {
        status[i] = readLong(1+4*i);
    }

    return status;
}

void SWEeprom::write()
{
    char oldVersion;
    long oldStatus[4];
    int  i;
    long value;

    oldVersion = EEPROM.read(0);
    for (i=0; i<4 ; i++) {
        oldStatus[i] = readLong(1+4*i);
    }
    
    if (oldVersion != version) {
        EEPROM.write(0,version);
    }
    for (i=0; i<4 ; i++) {
        if (oldStatus[i] != status[i]){
            value = status[i];
            EEPROM.write(4+4*i, value % 256);
            value = value / 256;
            EEPROM.write(3+4*i, value % 256);
            value = value / 256;
            EEPROM.write(2+4*i, value % 256);
            EEPROM.write(1+4*i, value / 256);
        }
    }
    EEPROM.commit();
    Ewritten = true;
}

long * SWEeprom::getStatus(){
    return status;
}

void SWEeprom::setStatus(int key, long value)
{
    Ewritten = false;
    status[key] = value;
}

void SWEeprom::decStatus(int key, long value)
{
    Ewritten = false;
    status[key] -= value;
}

void SWEeprom::incStatus(int key, long value)
{
    Ewritten = false;
    status[key] += value;
}

bool SWEeprom::written()
{
    return Ewritten;
}