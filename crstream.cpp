#include "crstream.h"

const PROGMEM char P_listen[]   = "listen"  ;
const PROGMEM char P_baud[]     = "baud="   ;
const PROGMEM char P_sleep[]    = "sleep="  ;
const PROGMEM char P_reset[]    = "reset"   ;
const PROGMEM char P_mhrtreq[]  = "mhrtreq" ;
const PROGMEM char P_send[]     = "send="   ;
const PROGMEM char P_sendto[]   = "sendto=" ;
const PROGMEM char P_sysreg[]   = "sysreg=" ;
const PROGMEM char P_mhdata[]   = "MHDATA"  ;

// Global functions & variables
char             _tempbuf[7];
const uint32_t   _baud[BAUDMODE_NUM] PROGMEM = {9600, 19200, 38400, 57600, 115200, 230400, 460800};
const char       _delim = ',';

int              asc2hex(char c) {
    if (isdigit(c))         c -= '0';
    else if (isalpha(c))    c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else return -1;

    if (c >= 16) return -1;
    else         return c;
}

uint8_t    bin2bcd (uint8_t val) {
    return val + 6 * (val / 10);
}

char    i2h(uint8_t i) {
    uint8_t k = i & 0x0F;
    if (k <= 9) {
        return '0' + k;
    } else {
        return 'A' + k - 10;
    }
}

void Status::clear() {
    sender          =     0 ;
    destination     =     0 ;
    id              =     0 ;
    rssi            =     0 ;
    length          =     0 ;
    available       =     0 ;
    fail            = false ;
    rxstate         = RX_INIT;
}

uint16_t crc16_update(uint16_t& crc, const char* const p_str) {
    for (unsigned int i = 0; i < strlen_P(p_str); i++) {
        char c = pgm_read_byte_near(p_str + i);
        crc ^= c;
        for (int i = 0; i < 8; ++i) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

uint16_t randomID(const char* const p_date, const char* const p_time) {
    uint16_t crc = 0xFFFF;
    crc16_update(crc, p_date);
    crc16_update(crc, p_time);
    return crc;
}

basic_crstream::basic_crstream(Stream& serial, uint32_t timeoutms)
    : serial   ( serial   )
    , _cin     ( serial   )
    , _timeoutms(timeoutms) {
    selfID      =         0 ;
    destID      =    0xFFFF ;
    panID       =         0 ;
    baudmode    = B_9600    ;
    datarate    = D_10K     ;
    channel     =        33 ;
    mhmode      = MHSLAVE   ;
    _isSleeping = false     ;
    _isWriting  = false     ;
}

void basic_crstream::update() {

    _rxProcess();
    // Process rx message
    if (_status.rxstate != _laststate) {
        _laststate  = _status.rxstate;
        _timems = millis();
        _isSleeping = false;
    } 
    // reset state when timeout
    else if (_status.rxstate != RX_INIT and _timeout()) {
        _clear();
        _timems = millis();
    }
    
    // complete any outstanding command
    if ( _isWriting ) {
        flush();
    }

    // If autosleep function is enabled, Cresson would go into sleep mode right after transmission.
    // It can not receive any message.
    if (autosleep and !_isSleeping) {
        writecmd(P_sleep, 1, 0x9999); // sleep 9999 seconds (BCD format)
        flush();
        _isSleeping = true;
    }
}

// Private functions
//-------------------------------------------
void basic_crstream::_rxProcess() {
    switch (_status.rxstate) {
        // find header
        case RX_INIT:
            if (serial.available() >= (int)sizeof(_tempbuf) ) {
                _cin.getline(_tempbuf, sizeof(_tempbuf), '\n');
                // compare to header, clear buffer if unmatched
                if (strcmp_P(_tempbuf, P_mhdata) == 0) {
                    _status.rxstate = RX_SENDER;
                } else {
                    _clear();
                }
            }
            break;

        case RX_SENDER:
            if (serial.available() >= 5 ) {
                _cin >> hex >> _status.sender;
                if ( !_cin.fail() ) {
                    _status.rxstate = RX_DEST;
                } else {
                    _clear();
                }
            }
            break;

        case RX_DEST:
            if (serial.available() >= 5 ) {
                _cin >> hex >> _status.destination;
                if ( !_cin.fail() ) {
                    _status.rxstate = RX_ID;
                } else {
                    _clear();
                }
            }
            break;

        case RX_ID:
            if (serial.available() >= 5 ) {
                _cin >> hex >> _status.id;
                if ( !_cin.fail() ) {
                    _status.rxstate = RX_RSSI;
                } else {
                    _clear();
                }
            }
            break;

        case RX_RSSI:
            if (serial.available() >= 5 ) {
                _cin >> dec >> _status.rssi;
                if ( !_cin.fail() ) {
                    _status.rxstate = RX_LENGTH;
                } else {
                    _clear();
                }
            }
            break;

        case RX_LENGTH:
            if (serial.available() >= 6 ) {
                _cin >> hex >> _status.length;
                if ( !_cin.fail() ) {
                    _cin.ignore(1, ' '); // skip whitespace
                    _status.rxstate = RX_PAYLOAD;
                } else {
                    _clear();
                }
            }
            break;

        case RX_PAYLOAD:
            if (_status.length/2 == _getnum) {
                _status.rxstate = RX_ROUTE;
            }
            break;

        case RX_ROUTE:
            _clear();
            break;

        default:
            break;
    }
}

int basic_crstream::_get () {
    int c1 = asc2hex(_cin.get());
    int c0 = asc2hex(_cin.get());

    if (c0 == -1 or c1 == -1)   return -1;
    else                        return (c1 << 4) | c0;
}

void basic_crstream::_write(const char* bytes, const uint8_t length) {
    for(uint8_t i = length; i != 0; i--) {
        serial.print( i2h(bytes[i-1] >> 4) );
        serial.print( i2h(bytes[i-1]     ) );
    }
}


void basic_crstream::writecmd(const char* const p_str, const uint16_t num, ...) {
    if (_isSleeping) {
        flush();
        flush();
        _isSleeping = false;
    }

    _isWriting = true;    
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

void basic_crstream::flush() {
    serial.println();
    serial.flush();
    _timems = millis();
    while (! _timeout()) {
        if (serial.available()) {
            _clearbuf();
            _timems = millis();
        }
    }
    _isWriting = false;
}

bool basic_crstream::_timeout() {
    return millis() - _timems > _timeoutms;
}
