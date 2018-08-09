/*
 * Copyright (C) 2018 CME Vietnam Co. Ltd.
 * v0.6.3 - Tuan Tran
*/
#include "crstream.h"

void cresson_onReceived()  __attribute__((weak));
void cresson_onReceiving() __attribute__((weak));
void cresson_sendFailed()  __attribute__((weak));
void cresson_wiringError() __attribute__((weak));
void cresson_onReceived()      {};
void cresson_onReceiving()     {};
void cresson_sendFailed()      {};
void cresson_wiringError()     {};

const PROGMEM char P_listen[]   = "listen"  ;
const PROGMEM char P_baud[]     = "baud="   ;
const PROGMEM char P_sleep[]    = "sleep="  ;
const PROGMEM char P_reset[]    = "reset"   ;
const PROGMEM char P_mhrtreq[]  = "mhrtreq" ;
const PROGMEM char P_send[]     = "send="   ;
const PROGMEM char P_sendto[]   = "sendto=" ;
const PROGMEM char P_sysreg[]   = "sysreg=" ;
const PROGMEM char P_mhdata[]   = "MHDATA"  ;
const PROGMEM char P_mhsend[]   = "MHSEND"  ;
const PROGMEM char P_mhack[]    = "MHACK:"  ;
const PROGMEM uint32_t   _baud[BAUDMODE_NUM] = {9600, 19200, 38400, 57600, 115200, 230400, 460800};

int        basic_crstream::asc2hex(char c) {
    if (isdigit(c))         c -= '0';
    else if (isalpha(c))    c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else return -1;

    if (c >= 16) return -1;
    else         return c;
}

uint8_t    basic_crstream::bin2bcd (uint8_t val) {
    return val + 6 * (val / 10);
}

char    basic_crstream::i2h(uint8_t i) {
    uint8_t k = i & 0x0F;
    if (k <= 9) {
        return '0' + k;
    } else {
        return 'A' + k - 10;
    }
}

basic_crstream::basic_crstream(Stream& serial)
    : serial   ( serial   ) {
    selfID                  =         0 ;
    destID                  = BROADCAST_ID ;
    panID                   =         0 ;
    baudmode                = BAUD_9600 ;
    datarate                = DATA_50K  ;
    channel                 =        33 ;
    mhmode                  = MHSLAVE   ;
    autosleep               = false     ;
    datamode                = BIN_MODE  ;
    _chrnum                 = 0         ;
    _currentState           = stIDLE    ;
    _txState                = stTX_IDLE ;
    _onReceivingFnc         = cresson_onReceiving;
    _onReceivedFnc          = cresson_onReceived ;
    _sendFailedFnc          = cresson_sendFailed ;
    _wiringErrorFnc         = cresson_wiringError;
    _clear();
}

void basic_crstream::_clear() {
    _chrnum                 = 0;
    _result.sender          = 0xFFFF;
    _result.destination     =     0 ;
    _result.msgid           =     0 ;
    _result.rssi            =     0 ;
    _result.length          =     0 ;
    _result.available       =     0 ;
    //_result.received        = false ;
    _result.decodeFailed    = false ;
    _result.sendAck         = -1    ;
    //_result.alive           = false;
}

bool basic_crstream::decodeFailed() {
    return _result.decodeFailed;
}

uint16_t basic_crstream::sender() {
    return _result.sender;
}

uint16_t basic_crstream::destination() {
    return _result.destination;
}

uint16_t basic_crstream::length() {
    return _result.length/2;
}

int16_t  basic_crstream::rssi()   {
    return _result.rssi;
}

uint16_t basic_crstream::available() {
    _result.available = min(_result.length/2 - _chrnum, (uint16_t)serial.available()/2); 
    return _result.available;
}

void  basic_crstream::onReceivedCallback    (void (*fnc)(void)) {
    _onReceivedFnc = fnc;   
}

void  basic_crstream::onReceivingCallback   (void (*fnc)(void)) {
    _onReceivingFnc = fnc;   
}

void  basic_crstream::sendFailedCallback    (void (*fnc)(void)) {
    _sendFailedFnc = fnc; 
}

void  basic_crstream::wiringErrorCallback   (void (*fnc)(void)) {
    _wiringErrorFnc = fnc; 
}

bool  basic_crstream::isAlive  () {
    bool org_autosleep = autosleep;
    uint16_t org_destID = destID;
    autosleep = false;
    destID = selfID;
    (*this) << _delim;
    listen();
    autosleep = org_autosleep;
    destID = org_destID;
    return _result.alive;
}

// Listen for a message until timeout.
bool basic_crstream::listen(uint32_t timems) {
    if ( _txState == stTX_WRITE_DATA ) {
        if (_currentState != stIDLE) _listen();
        serial.println();
        serial.flush();
        _switchSM( stTX_HEADER_MHSEND );
        _txState = stTX_IDLE;
        _result.alive = true;
    }
    bool received = _listen(timems);
    if (autosleep and _currentState != stSLEEP) sleep();
    return received;
};

// write command API
void basic_crstream::writecmd(const char* const p_str, const uint16_t num, ...) {
    if  (_currentState == stSLEEP) wakeup();

    for (unsigned int i = 0; i < strlen_P(p_str); i++) {
        char c = pgm_read_byte_near(p_str + i);
        serial.print(c);
    }
    
    va_list     args;
    va_start(args, num);
    for(uint8_t i = 0; i < num; i++){
        if (i!=0) serial.print(_delim);
        uint16_t arg = va_arg(args, uint16_t);
        _write((char*) &arg, sizeof(uint16_t));
    }
    va_end(args);
}

// wait for rx buffer clear
void basic_crstream::execute() {
    serial.println();
    serial.flush();
    _switchSM( stWAIT_FOR_IDLE );
    _listen();
}

// wake from sleep
void basic_crstream::wakeup() {
    execute();
    execute();
}

// sleep 9999 secs
void basic_crstream::sleep() {
    writecmd(P_sleep, 1, 0x9999); // sleep 9999 seconds (BCD format)
    execute();
    _switchSM( stSLEEP );
}

bool basic_crstream::_timeout(uint32_t timeoutms) {
    return millis() - _timems > timeoutms;
}

// switching machine state
void basic_crstream::_switchSM(uint8_t newState) {
    _currentState = newState;
    _timems = millis();
}

// readout a character from serial buffer
int basic_crstream::_getchr () {
    int c1 = asc2hex((char)serial.read());
    int c0 = asc2hex((char)serial.read());

    if (c0 == -1 or c1 == -1)   return -1;
    else                        return (c1 << 4) | c0;
}

// readout a int/uint from serial buffer
int basic_crstream::_getint () {
    int i1 = _getchr();
    int i0 = _getchr();

    if (i0 == -1 or i1 == -1)   {
        _result.decodeFailed = true;
        return -1;
    } else {
        return (i1 << 8) | i0;
    }
}

// write hexascii
void basic_crstream::_write(const char* bytes, const uint8_t length) {
    for(uint8_t i = length; i != 0; i--) {
        serial.print( i2h(bytes[i-1] >> 4) );
        serial.print( i2h(bytes[i-1]     ) );
    }
}

// find header
bool basic_crstream::_headerMatched(const char* const p_str) {
    if (serial.available() >= (int)sizeof(_tempbuf) ) {
        if ( serial.peek() == 'M') {
            serial.readBytesUntil(' ', _tempbuf, 6);
            _tempbuf[6] = '\0';
            if (strcmp_P(_tempbuf, p_str) == 0) return true;
        } else {
            serial.read(); // skip the character if it's not 'M'
        }
    }
    return false;
}

// The function would stop if cresson in IDLE state, AND any of following condition met:
//   + timeout
//   + message received
bool basic_crstream::_listen(uint32_t timems) {
    uint32_t localtime = millis();
    do {
        _update();
        if (_currentState == stSLEEP                                 ) break;
        if (_currentState == stIDLE and _result.received             ) break;
        if (_currentState == stIDLE and millis() - localtime > timems) break;
    } while ( true );
    bool received = _result.received;
    _result.received = false;
    return received;
};

// Private functions
//-------------------------------------------
void basic_crstream::_update() {
    switch (_currentState) {
        // IDLE state: wait for message / command
        case stIDLE:
            if (serial.available()) _switchSM( stRX_HEADER );
            break;

        // low-power
        case stSLEEP:
            if (serial.available()) _switchSM( stWAIT_FOR_IDLE );
            break;

        // find MHDATA
        case stRX_HEADER:
            if (_headerMatched(P_mhdata)) {
                serial.read(); // skip whitespace
                _switchSM( stRX_SENDER );
            }
            if (_timeout()) _switchSM( stWAIT_FOR_IDLE );
            break;

        // find sender ID
        case stRX_SENDER:
            if (serial.available() >= 5 ) {
                _result.sender = _getint();
                if ( !_result.decodeFailed ) {
                    serial.read(); // skip whitespace
                    _switchSM( stRX_DEST        );
                } else {
                    _switchSM( stWAIT_FOR_IDLE  );
                }
            }
            if (_timeout()) _switchSM( stWAIT_FOR_IDLE );
            break;

        // find dest ID
        case stRX_DEST:
            if (serial.available() >= 5 ) {
                _result.destination = _getint();
                if ( !_result.decodeFailed ) {
                    serial.read(); // skip whitespace
                    _switchSM( stRX_MSGID       );
                } else {
                    _switchSM( stWAIT_FOR_IDLE  );
                }
            }
            if (_timeout()) _switchSM( stWAIT_FOR_IDLE );
            break;

        // find message ID
        case stRX_MSGID:
            if (serial.available() >= 5 ) {
                _result.msgid = _getint();
                if ( !_result.decodeFailed ) {
                    serial.read(); // skip whitespace
                    _switchSM( stRX_RSSI        );
                } else {
                    _switchSM( stWAIT_FOR_IDLE  );
                }
            }
            if (_timeout()) _switchSM( stWAIT_FOR_IDLE );
            break;

        // find RSSI
        case stRX_RSSI:
            if (serial.available() >= 5 ) {
                _result.rssi = serial.parseInt();
                if ( _result.rssi <=0 ) {
                    serial.read(); // skip whitespace
                    _switchSM( stRX_LENGTH      );
                } else {
                    _switchSM( stWAIT_FOR_IDLE  );
                }
            }
            if (_timeout()) _switchSM( stWAIT_FOR_IDLE );
            break;

        // find message length
        case stRX_LENGTH:
            if (serial.available() >= 6 ) {
                _result.length =  _getint();
                if ( !_result.decodeFailed ) {
                    serial.read(); // skip whitespace
                    _switchSM( stRX_PAYLOAD );
                } else {
                    _switchSM( stWAIT_FOR_IDLE );
                }
            }
            if (_timeout()) _switchSM( stWAIT_FOR_IDLE );
            break;

        // buffering payload
        case stRX_PAYLOAD:
            if ((uint16_t)serial.available() > _result.length - _chrnum*2) {
                _result.received = true;
            }
            if (available() and _onReceivingFnc) _onReceivingFnc();
            if (_result.received) {
            	if (_onReceivedFnc) _onReceivedFnc();
            	_switchSM( stWAIT_FOR_IDLE );
            }
            if (_timeout(3000)) _switchSM( stWAIT_FOR_IDLE );
            break;

        // find MHSEND
        case stTX_HEADER_MHSEND:
            if (_headerMatched(P_mhsend)) {
                if (destID != BROADCAST_ID and destID != selfID) {
                    _switchSM( stTX_HEADER_MHACK );
                } else {
                    _result.sendAck = 0;
                    _switchSM( stWAIT_FOR_IDLE    );
                }
            }
            if (_timeout(100)) {
                _result.alive = false;
                if (_wiringErrorFnc) _wiringErrorFnc();
                _switchSM( stWAIT_FOR_IDLE );
            }
            break;

        // find MHACK
        case stTX_HEADER_MHACK:
            if (_headerMatched(P_mhack)) _switchSM( stTX_RESPOND );
            if (_timeout(3000)) {
                if (_sendFailedFnc) _sendFailedFnc();
                _switchSM( stWAIT_FOR_IDLE );
            }
            break;

        // MHACK value
        case stTX_RESPOND:
            if (serial.available() >= 3 ) {
                _result.sendAck = _getchr();
                if ( _result.sendAck != 0 ) {
                    if (_sendFailedFnc) _sendFailedFnc();
                }
                _switchSM( stWAIT_FOR_IDLE  );
            }
            if (_timeout()) {
                if (_sendFailedFnc) _sendFailedFnc();
                _switchSM( stWAIT_FOR_IDLE );
            }
            break;

        // wait for idle
        case stWAIT_FOR_IDLE:
        default:
            while (serial.available()) {
                serial.read();
                _timems = millis();
            }
            if (_timeout()) {
                _clear();
                _switchSM( stIDLE );
            }
            break;
    }
}