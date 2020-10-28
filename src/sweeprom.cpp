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
        if (oldVersion <= 0) {
            // 'default' values, upgrade to version 1
            status[0]  = 50;
            status[1]  = 4;
            status[2]  = 3;
            status[3]  = 2;
        } else if (oldVersion == 1) {
            // upgrade to version 2
            ssid[0] = '\0';
            psk[0]  = '\0';
            hostname[0] = '\0';
        }
        version = EEPROMVERSION;
        write();
    } else {
        read();
    }

    return status;
}

long * SWEeprom::read()
{
    int i,len;
    version   = EEPROM.read(0);
    // next starts at address 1
    for (i=0; i<4 ; i++) {
        // long is 4 bytes
        status[i] = readLong(1+4*i);
    }
    // next starts at address 1 + 4 + 4 + 4 + 4 = 17
    // read ssid
    for (i=0; i<32; i++){
        ssid[i]=EEPROM.read(17+i);
    }
    len = strlen(ssid);
    if (len > 31 ) ssid[31] = '\0';
    // next starts at addres 17+32 = 49
    // read psk 
    for (i=0; i<64; i++){
        psk[i]=EEPROM.read(49+i);
    }
    len = strlen(psk);
    if (len > 63 ) psk[63] = '\0';
    // next address at 49 + 64 = 113
    // read hostname
    for (i=0; i<32; i++){
        ssid[i]=EEPROM.read(113+i);
    }
    len = strlen(hostname);
    if (len > 31 ) hostname[31] = '\0';
    // next address at 113 + 32 = 145
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
    // next starts at address 1 + 4 + 4 + 4 + 4 = 17
    // read ssid
    for (i=0; i<32; i++){ EEPROM.write(17+i,ssid[i]); }
    // next starts at addres 17+32 = 49
    // read psk 
    for (i=0; i<64; i++){ EEPROM.write(49+i,psk[i]); }
    // next starts at address 49 + 63 = 113
    // read hostname
    for (i=0; i<32; i++) { EEPROM.write(113+i,hostname[i]); }
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

void SWEeprom::setSSID(char * newSSID)
{
    int len;
    Ewritten = false;
    len = strlen(newSSID);
    if (len > 31) len = 31;
    strncpy(ssid,newSSID,len);
    ssid[len+1] = '\0';
    // copy: ssid = newSSID;
}

void SWEeprom::setPSK(char * newPSK)
{
    int len;
    Ewritten = false;
    len = strlen(newPSK);
    if (len > 63) len = 63;
    strncpy(psk,newPSK,len);
    psk[len+1] = '\0';
    // copy: ssid = newPSK;
}

void SWEeprom::setHostname(char * newHostname)
{
    int len;
    Ewritten = false;
    len = strlen(newHostname);
    if (len > 31) len = 31;
    strncpy(hostname,newHostname,len);
    hostname[len+1] = '\0';
    // copy: ssid = newSSID;
}