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
        
    private:
        char version;
        long status[4];
        bool Ewritten;
};

#endif
