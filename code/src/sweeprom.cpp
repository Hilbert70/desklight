/*
 * sweeprom.cpp
 *
 * just the functions
 */

#include "sweeprom.h"


//long readLong(int addres){
//    return ( (EEPROM.read(addres)*256+EEPROM.read(1+addres))*256 + EEPROM.read(2+addres))*256 + EEPROM.read(3+addres);
//}

SWEeprom::SWEeprom()
{
 
    Ewritten = false;
}

long * SWEeprom::init()
{
    uint8_t oldVersion;

    // make sure we can do something with the nvs
    errorCode = nvs_flash_init();
    if (errorCode != ESP_OK) {
        errorMessage = "Warning: NVS. Cannot init flash mem";
        if (errorCode != ESP_ERR_NVS_NO_FREE_PAGES) {
            return NULL;
        }
        // erase and reinit
        errorMessage = "NVS. Try reinit the partition";
        const esp_partition_t *nvs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
        if (nvs_partition == NULL) {
            return NULL;
        }
        errorCode = esp_partition_erase_range(nvs_partition, 0, nvs_partition->size);
        errorCode = nvs_flash_init();
        if (errorCode) {
            return NULL;
        }
        errorMessage = "NVS. Partition re-formatted";
    }
    errorCode = nvs_open("desklight", NVS_READWRITE, &_nvs_handle);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: Failed to open";
        return NULL;
    }

    errorCode = nvs_get_u8(_nvs_handle, "version", &oldVersion);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read version";
        return NULL;
    }

    if (oldVersion != EEPROMVERSION) {
        if (oldVersion <= 0) {
            // 'default' values, upgrade to version 1
            status[0]  = 50;
            status[1]  = 4;
            status[2]  = 3;
            status[3]  = 2;
            ssid[0] = '\0';
            psk[0]  = '\0';
            hostname[0] = '\0';
        } else {
            read();
            if (oldVersion == 1) {
                // upgrade to version 2
                ssid[0] = '\0';
                psk[0]  = '\0';
                hostname[0] = '\0';
            }
        }    
        version = EEPROMVERSION;
        write();
    } else {
        read();
    }
    Ewritten = true;
    
    return status;
}

long * SWEeprom::read()
{
    int len;
    uint32_t statusValue;
    size_t required_size;

    errorCode = nvs_get_u8(_nvs_handle, "version", &version);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read version";
        return NULL;
    }
    // yup the status code is durty
    errorCode = nvs_get_u32(_nvs_handle, "status0", &statusValue);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read status0";
        return NULL;
    }
    status[0]= (long )statusValue;
    errorCode = nvs_get_u32(_nvs_handle, "status1", &statusValue);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read status1";
        return NULL;
    }
    status[1]= (long )statusValue;
    errorCode = nvs_get_u32(_nvs_handle, "status2", &statusValue);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read status2";
        return NULL;
    }
    status[2]= (long )statusValue;
    errorCode = nvs_get_u32(_nvs_handle, "status3", &statusValue);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read status3";
        return NULL;
    }
    status[3]= (long )statusValue;

    // read ssid
    errorCode = nvs_get_str(_nvs_handle, "ssid", NULL, &required_size);
    errorCode = nvs_get_str(_nvs_handle, "ssid", ssid, &required_size);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read ssid";
        return NULL;
    }
    len = strlen(ssid);
    if (len > 31 ) { len = 31; }
    ssid[len] = '\0';

    // read psk
    errorCode = nvs_get_str(_nvs_handle, "psk", NULL, &required_size); 
    errorCode = nvs_get_str(_nvs_handle, "psk", psk, &required_size);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read psk";
        return NULL;
    }
    len = strlen(psk);
    if (len > 63 ) { len = 63; }
    psk[len] = '\0';
    // next address at 49 + 64 = 113
    // read hostname
    errorCode = nvs_get_str(_nvs_handle, "hostname", NULL, &required_size);
    errorCode = nvs_get_str(_nvs_handle, "hostname", hostname, &required_size);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to read hostname";
        return NULL;
    }
    len = strlen(hostname);
    if (len > 31 ) { len = 31; }
    hostname[len] = '\0';
    // next address at 113 + 32 = 145
    return status;
}

void SWEeprom::write()
{
    errorCode = nvs_set_u8(_nvs_handle, "version", version);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to write version";
        return;
    }
    // yup the status code is durty
    errorCode = nvs_set_u32(_nvs_handle, "status0", (uint32_t )status[0]);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to write status0";
        return;
    }
    errorCode = nvs_set_u32(_nvs_handle, "status1", (uint32_t )status[1]);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to write status1";
        return;
    }
    errorCode = nvs_set_u32(_nvs_handle, "status2", (uint32_t )status[2]);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to write status2";
        return;
    }
    errorCode = nvs_set_u32(_nvs_handle, "status3", (uint32_t )status[3]);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to write status3";
        return;
    }

    // write ssid
    errorCode = nvs_set_str(_nvs_handle, "ssid", ssid);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to write ssid";
        return;
    }

    // write psk 
    errorCode = nvs_set_str(_nvs_handle, "psk", psk);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to write psk";
        return;
    }
    // write hostname
    errorCode = nvs_set_str(_nvs_handle, "hostname", hostname);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to write hostname";
        return;
    }
    errorCode  = nvs_commit(_nvs_handle);
    if (errorCode != ESP_OK) {
        errorMessage = "NVS: failed to commit";
    }
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

void SWEeprom::setSSID(String newSSID)
{
    int len,i;
    Ewritten = false;
    len = newSSID.length();
    if (len > 31) len = 31;
    for (i=0; i< 31; i++) {
        ssid[i] = newSSID[i];
    }
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

void SWEeprom::setPSK(String newPSK)
{
    int len,i;
    Ewritten = false;
    len = newPSK.length();
    if (len > 63) len = 63;
    for (i=0; i< 31; i++) {
        psk[i] = newPSK[i];
    }
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

void SWEeprom::setHostname(String newHostname)
{
    int len,i;
    Ewritten = false;
    len = newHostname.length();
    if (len > 31) len = 31;
    for (i=0; i< 31; i++) {
        hostname[i] = newHostname[i];
    }
    hostname[len+1] = '\0';
    // copy: ssid = newSSID;
}

char * SWEeprom::getSSID(){
    return ssid;
}

char * SWEeprom::getPSK(){
    return psk;
}

char * SWEeprom::getHostname(){
    return hostname;
}