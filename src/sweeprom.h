#ifndef __SWEEPROM_H
#define __SWEEPROM_H

/*
 * sweerpom.h
 *
 * structure of the eeporm and support functions
 */
#include <EEPROM.h>

#define EEPROMVERSION 2

#define ST_DIM 0
#define ST_START 1
#define ST_LENGTH 2
#define ST_LEDS 3

class SWEeprom {
    public:
        SWEeprom();
        
        long * init();
        long * read();
        void write();
        long * getStatus();
        void setStatus(int key, long value);
        void incStatus(int key, long value);
        void decStatus(int key, long value);
        bool written();
        void setSSID(char * newSSID);
        void setPSK(char * newPSK);
        void setHostname(char * newHostname);

        // just returns the pointer, you are not the owner ;-)
        char * getSSID();
        char * getPSK();
        char * getHostname();
        
    private:
        // in eeporm (flash)
        char version;
        long status[4];
        char ssid[32];     // wifi ssid
        char psk[64];      // wifi password
        char hostname[32]; // limit hostname to 32 chars
        // class helper varables
        bool Ewritten;
};

#endif
