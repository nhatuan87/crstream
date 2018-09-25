/*
 * Copyright (C) 2018 CME Vietnam Co. Ltd.
 * v0.2 - Tuan Tran
*/
#ifndef UNIQUEID_H
#define UNIQUEID_H

#include <Arduino.h>
#include <avr/boot.h>

const PROGMEM char P_date[]   = __DATE__  ;
const PROGMEM char P_time[]   = __TIME__  ;
uint16_t _crc16_update(uint16_t crc, uint8_t a) {
    int i;

    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
        else
        crc = (crc >> 1);
    }

    return crc;
};

uint16_t crc16_calc(uint16_t& crc, const char* const p_str) {
    for (unsigned int i = 0; i < strlen_P(p_str); i++) {
        uint8_t a = pgm_read_byte_near(p_str + i);
        crc = _crc16_update(crc, a);
    }
    return crc;
}

uint16_t uniqueID() {
#if defined(__AVR_ATmega328PB__)
    return boot_signature_byte_get(0x16) << 8 |  boot_signature_byte_get(0x17);
#else
    uint16_t crc = 0xFFFF;
    crc16_calc(crc, P_date);
    crc16_calc(crc, P_time);
    return crc;
#endif
}

#endif