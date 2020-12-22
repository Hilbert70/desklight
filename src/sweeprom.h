#ifndef __SWEEPROM_H
#define __SWEEPROM_H

/*
 * sweerpom.h
 *
 * structure of the eeporm and support functions
 */
#include <arduino.h>

extern "C" {
#include "esp_partition.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
}

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
        void setSSID(String newSSID);
        void setPSK(String newPSK);
        void setHostname(String newHostname);

        // just returns the pointer, you are not the owner ;-)
        char * getSSID();
        char * getPSK();
        char * getHostname();
        
        String errorMessage;
        esp_err_t    errorCode;       // mostly nvs error codes
    private:
        // in eeporm (flash)
        uint8_t version;
        long status[4];
        char ssid[32];     // wifi ssid
        char psk[64];      // wifi password
        char hostname[32]; // limit hostname to 32 chars
        // class helper varables
        bool Ewritten;
    protected:
        nvs_handle  _nvs_handle;
};

#endif
