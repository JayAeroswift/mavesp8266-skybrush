/****************************************************************************
 *
 * Copyright (c) 2015, 2016 Gus Grubba. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file mavesp8266_parameters.cpp
 * ESP8266 Wifi AP, MavLink UART/UDP Bridge
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "mavesp8266.h"
#include "mavesp8266_parameters.h"
#include "crc.h"

const char* kDEFAULT_SSDI       = "PixRacer";
const char* kDEFAULT_PASSWORD   = "pixracer";

//-- Reserved space for EEPROM persistence. A change in this will cause all values to reset to defaults.
#define EEPROM_SPACE            32 * sizeof(uint32_t)
#define EEPROM_CRC_ADD          EEPROM_SPACE - (sizeof(uint32_t) << 1)

uint32_t    _sw_version;
int8_t      _debug_enabled;
uint32_t    _wifi_channel;
uint16_t    _wifi_udp_hport;
uint16_t    _wifi_udp_cport;
char        _wifi_ssid[16];
char        _wifi_password[16];
uint32_t    _uart_baud_rate;
uint32_t    _flash_left;

//-- Parameters
//   No string support in parameters so we stash a char[16] into 4 uint32_t
 struct stMavEspParameters mavParameters[] = {
     {"SW_VER",          &_sw_version,          MavESP8266Parameters::ID_FWVER,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  true },
     {"DEBUG_ENABLED",   &_debug_enabled,       MavESP8266Parameters::ID_DEBUG,     sizeof(int8_t),     MAV_PARAM_TYPE_INT8,    false},
     {"WIFI_CHANNEL",    &_wifi_channel,        MavESP8266Parameters::ID_CHANNEL,   sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_UDP_HPORT",  &_wifi_udp_hport,      MavESP8266Parameters::ID_HPORT,     sizeof(uint16_t),   MAV_PARAM_TYPE_UINT16,  false},
     {"WIFI_UDP_CPORT",  &_wifi_udp_cport,      MavESP8266Parameters::ID_CPORT,     sizeof(uint16_t),   MAV_PARAM_TYPE_UINT16,  false},
     {"WIFI_SSID1",      &_wifi_ssid[0],        MavESP8266Parameters::ID_SSID1,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSID2",      &_wifi_ssid[4],        MavESP8266Parameters::ID_SSID2,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSID3",      &_wifi_ssid[8],        MavESP8266Parameters::ID_SSID3,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSID4",      &_wifi_ssid[12],       MavESP8266Parameters::ID_SSID4,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PASSWORD1",  &_wifi_password[0],    MavESP8266Parameters::ID_PASS1,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PASSWORD2",  &_wifi_password[4],    MavESP8266Parameters::ID_PASS2,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PASSWORD3",  &_wifi_password[8],    MavESP8266Parameters::ID_PASS3,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PASSWORD4",  &_wifi_password[12],   MavESP8266Parameters::ID_PASS4,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"UART_BAUDRATE",   &_uart_baud_rate,      MavESP8266Parameters::ID_UART,      sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false}
};

//---------------------------------------------------------------------------------
MavESP8266Parameters::MavESP8266Parameters()
{
}

//---------------------------------------------------------------------------------
//-- Fail safe
uint32_t bogusVar = 0;
struct stMavEspParameters bogus = {"ERROR", &bogusVar, MavESP8266Parameters::ID_COUNT, sizeof(uint32_t), MAV_PARAM_TYPE_UINT32, true};

//---------------------------------------------------------------------------------
//-- Initialize
void
MavESP8266Parameters::begin()
{
    EEPROM.begin(EEPROM_SPACE);
    _initEeprom();
}

//---------------------------------------------------------------------------------
//-- Array accessor
stMavEspParameters*
MavESP8266Parameters::getAt(int index)
{
    if(index < ID_COUNT)
        return &mavParameters[index];
    else
        return &bogus;
}

//---------------------------------------------------------------------------------
//-- Parameters
uint32_t    MavESP8266Parameters::getSwVersion      () { return _sw_version;        }
int8_t      MavESP8266Parameters::getDebugEnabled   () { return _debug_enabled;     }
uint32_t    MavESP8266Parameters::getWifiChannel    () { return _wifi_channel;      }
uint16_t    MavESP8266Parameters::getWifiUdpHport   () { return _wifi_udp_hport;    }
uint16_t    MavESP8266Parameters::getWifiUdpCport   () { return _wifi_udp_cport;    }
char*       MavESP8266Parameters::getWifiSsid       () { return _wifi_ssid;         }
char*       MavESP8266Parameters::getWifiPassword   () { return _wifi_password;     }
uint32_t    MavESP8266Parameters::getUartBaudRate   () { return _uart_baud_rate;    }

//---------------------------------------------------------------------------------
//-- Reset all to defaults
void
MavESP8266Parameters::resetToDefaults()
{
    _sw_version        = MAVESP8266_VERSION;
    _debug_enabled     = 0;
    _wifi_channel      = DEFAULT_WIFI_CHANNEL;
    _wifi_udp_hport    = DEFAULT_UDP_HPORT;
    _wifi_udp_cport    = DEFAULT_UDP_CPORT;
    _uart_baud_rate    = DEFAULT_UART_SPEED;
    strncpy(_wifi_ssid, kDEFAULT_SSDI, sizeof(_wifi_ssid));
    strncpy(_wifi_password, kDEFAULT_PASSWORD, sizeof(_wifi_password));
    _flash_left = ESP.getFreeSketchSpace();
}

//---------------------------------------------------------------------------------
//-- Saves all parameters to EEPROM
void
MavESP8266Parameters::loadAllFromEeprom()
{
    uint32_t address = 0;
    for(int i = 0; i < ID_COUNT; i++) {
        uint8_t* ptr = (uint8_t*)mavParameters[i].value;
        for(int j = 0; j < mavParameters[i].length; j++, address++, ptr++) {
            *ptr = EEPROM.read(address);
        }
        #ifdef DEBUG
            Serial1.print("Loading from EEPROM: ");
            Serial1.print(mavParameters[i].id);
            Serial1.print(" Value: ");
            if(mavParameters[i].type == MAV_PARAM_TYPE_UINT32)
                Serial1.println(*((uint32_t*)mavParameters[i].value));
            else if(mavParameters[i].type == MAV_PARAM_TYPE_UINT16)
                Serial1.println(*((uint16_t*)mavParameters[i].value));
            else
                Serial1.println(*((int8_t*)mavParameters[i].value));
        #endif
    }
    #ifdef DEBUG
        Serial1.println("");
    #endif
    //-- Version if hardwired
    _sw_version = MAVESP8266_VERSION;
    _flash_left = ESP.getFreeSketchSpace();
}

//---------------------------------------------------------------------------------
//-- Compute parameters hash
uint32_t MavESP8266Parameters::paramHashCheck()
{
    uint32_t crc = 0;
    for(int i = 0; i < ID_COUNT; i++) {
        crc = _crc32part((uint8_t *)(void*)mavParameters[i].id, strlen(mavParameters[i].id), crc);
        //-- QGC Expects a CRC of sizeof(uint32_t)
        uint32_t val = 0;
        if(mavParameters[i].type == MAV_PARAM_TYPE_UINT32)
            val = *((uint32_t*)mavParameters[i].value);
        else if(mavParameters[i].type == MAV_PARAM_TYPE_UINT16)
            val = (uint32_t)*((uint16_t*)mavParameters[i].value);
        else
            val = (uint32_t)*((int8_t*)mavParameters[i].value);
        crc = _crc32part((uint8_t *)(void*)&val, sizeof(uint32_t), crc);
    }
    delay(0);
    return crc;
}

//---------------------------------------------------------------------------------
//-- Saves all parameters to EEPROM
void
MavESP8266Parameters::saveAllToEeprom()
{
    //-- Init flash space
    uint8_t* ptr = EEPROM.getDataPtr();
    memset(ptr, 0, EEPROM_SPACE);
    //-- Write all paramaters to flash
    uint32_t address = 0;
    for(int i = 0; i < ID_COUNT; i++) {
        ptr = (uint8_t*)mavParameters[i].value;
        #ifdef DEBUG
            Serial1.print("Saving to EEPROM: ");
            Serial1.print(mavParameters[i].id);
            Serial1.print(" Value: ");
            if(mavParameters[i].type == MAV_PARAM_TYPE_UINT32)
                Serial1.println(*((uint32_t*)mavParameters[i].value));
            else if(mavParameters[i].type == MAV_PARAM_TYPE_UINT16)
                Serial1.println(*((uint16_t*)mavParameters[i].value));
            else
                Serial1.println(*((int8_t*)mavParameters[i].value));
        #endif
        for(int j = 0; j < mavParameters[i].length; j++, address++, ptr++) {
            EEPROM.write(address, *ptr);
        }
    }
    uint32_t saved_crc = _getEepromCrc();
    EEPROM.put(EEPROM_CRC_ADD, saved_crc);
    EEPROM.commit();
    #ifdef DEBUG
        Serial1.print("Saved CRC: ");
        Serial1.print(saved_crc);
        Serial1.println("");
    #endif
}

//---------------------------------------------------------------------------------
//-- Compute CRC32 for a buffer
uint32_t
MavESP8266Parameters::_crc32part(uint8_t* src, uint32_t len, uint32_t crc)
{
    for (int i = 0;  i < len;  i++) {
        crc = crc_table[(crc ^ src[i]) & 0xff] ^ (crc >> 8);
    }
    return crc;
}

//---------------------------------------------------------------------------------
//-- Computes EEPROM CRC
uint32_t
MavESP8266Parameters::_getEepromCrc()
{
    uint32_t crc  = 0;
    uint32_t size = 0;
    //-- Get size of all parameter data
    for(int i = 0; i < ID_COUNT; i++) {
        size += mavParameters[i].length;
    }
    //-- Computer CRC
    for (int i = 0 ; i < size; i++) {
        crc = crc_table[(crc ^ EEPROM.read(i)) & 0xff] ^ (crc >> 8);
    }
    return crc;
}

//---------------------------------------------------------------------------------
//-- Initializes EEPROM. If not initialized, set to defaults and save it.
void
MavESP8266Parameters::_initEeprom()
{
    loadAllFromEeprom();
    //-- Is it uninitialized?
    uint32_t saved_crc = 0;
    EEPROM.get(EEPROM_CRC_ADD, saved_crc);
    uint32_t current_crc = _getEepromCrc();
    if(saved_crc != current_crc) {
        #ifdef DEBUG
            Serial1.print("Initializing EEPROM. Saved: ");
            Serial1.print(saved_crc);
            Serial1.print(" Current: ");
            Serial1.println(current_crc);
        #endif
        //-- Set all defaults
        resetToDefaults();
        //-- Save it all and store CRC
        saveAllToEeprom();
    } else {
        //-- Load all parameters from EEPROM
        loadAllFromEeprom();
    }
}

//---------------------------------------------------------------------------------
// Stupid trick to find out how much RAM is left
uint32_t
MavESP8266Parameters::availableMemory()
{
    //-- Assume a max of 100k
    int size = 1024 * 100;
    byte* buf;
    while((buf = (byte *) malloc(size)) == NULL)
        size -= 128;
    free(buf);
    return size;
}

//---------------------------------------------------------------------------------
void
MavESP8266Parameters::setDebugEnabled(int8_t enabled)
{
    _debug_enabled     = enabled;
}

//---------------------------------------------------------------------------------
void
MavESP8266Parameters::setWifiChannel(uint32_t channel)
{
    _wifi_channel      = channel;
}

//---------------------------------------------------------------------------------
void
MavESP8266Parameters::setWifiUdpHport(uint16_t port)
{
    _wifi_udp_hport    = port;
}

//---------------------------------------------------------------------------------
void
MavESP8266Parameters::setWifiUdpCport(uint16_t port)
{
    _wifi_udp_cport    = port;
}

//---------------------------------------------------------------------------------
void
MavESP8266Parameters::setWifiSsid(const char* ssid)
{
    strncpy(_wifi_ssid, ssid, sizeof(_wifi_ssid));
}

//---------------------------------------------------------------------------------
void
MavESP8266Parameters::setWifiPassword(const char* pwd)
{
    strncpy(_wifi_password, pwd, sizeof(_wifi_password));
}

//---------------------------------------------------------------------------------
void
MavESP8266Parameters::setUartBaudRate(uint32_t baud)
{
    _uart_baud_rate = baud;
}