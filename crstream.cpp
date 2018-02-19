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

MinimumSerial dbgSerial;

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

char    i2h(uint8_t i)
{
    uint8_t k = i & 0x0F;
    if (k <= 9) {
        return '0' + k;
    } else {
        return 'A' + k - 10;
    }
}

basic_crstream::basic_crstream(Stream& serial)
    : serial   ( serial   )
    , _cin      ( serial   )
{
    selfID      =         0 ;
    destID      =    0xFFFF ;
    panID       =         0 ;
    baudmode    = B_9600    ;
    datarate    = D_10K     ;
    channel     =        33 ;
    mhmode      = MHSLAVE   ;
    _isSleeping = false     ;
    _isWriting  = false     ;
    _isSending  = false     ;
    _rxstate    = RX_INIT   ;
}

void basic_crstream::clear() {
    _cin.ignore(_serialAvailable, '\n');
    _status.sender          =     0 ;
    _status.destination     =     0 ;
    _status.id              =     0 ;
    _status.rssi            =     0 ;
    _status.length          =     0 ;
    _status.ready           = false ;
    _status.fail            = false ;
    _serialAvailable        = 0     ;
    _rxstate                = RX_INIT;
}

void basic_crstream::update() {
    _isSending = false;

    if (serial.available()) {
        _isSleeping = false;
        _rxProcess();
    }

    if (_isWriting        ) {
        _isWriting = false;
        serial.println();
        _timems = millis();
    }

     // the cresson waked from _sleep, put it into _sleep mode again
    if (autosleep and !_isSleeping and _timeout()) _sleep();
}

void basic_crstream::_sleep() {
    writecmd(P_sleep, 1, 0x9999); // it's 9999 seconds (BCD format)
    flush();
    _isSleeping = true;
}

// Private functions
//-------------------------------------------
void basic_crstream::_rxProcess() {
     // automatically clear buffer if there is no processing

    if (serial.available() != _serialAvailable) {
        _serialAvailable  = serial.available();
        _timems = millis();
    } else if (_timeout()) {
        clear();
    }

    // reset flag to goodbit
    _cin.clear();
    switch (_rxstate) {
        // find MHDATA
        case RX_INIT:
            if (serial.available() >= (int)sizeof(_tempbuf) ) {
                _cin.getline(_tempbuf, sizeof(_tempbuf), ' ');
                if (strcmp_P(_tempbuf, P_mhdata) == 0) _rxstate = RX_SENDER;
                else _cin.ignore(serial.available(), '\n');
            }
            break;

        case RX_SENDER:
            if (serial.available() >= 5 ) {
                _cin >> hex >> _status.sender;
                if ( !_cin.fail() ) {
                    _rxstate = RX_DEST;
                } else {
                    _cin.ignore(serial.available(), '\n');
                    _rxstate = RX_INIT;
                }
            }
            break;

        case RX_DEST:
            if (serial.available() >= 5 ) {
                _cin >> hex >> _status.destination;
                if ( !_cin.fail() ) {
                    _rxstate = RX_ID;
                } else {
                    _cin.ignore(serial.available(), '\n');
                    _rxstate = RX_INIT;
                }
            }
            break;

        case RX_ID:
            if (serial.available() >= 5 ) {
                _cin >> hex >> _status.id;
                if ( !_cin.fail() ) {
                    _rxstate = RX_RSSI;
                } else {
                    _cin.ignore(serial.available(), '\n');
                    _rxstate = RX_INIT;
                }
            }
            break;

        case RX_RSSI:
            if (serial.available() >= 5 ) {
                _cin >> dec >> _status.rssi;
                if ( !_cin.fail() ) {
                    _rxstate = RX_LENGTH;
                } else {
                    _cin.ignore(serial.available(), '\n');
                    _rxstate = RX_INIT;
                }
            }
            break;

        case RX_LENGTH:
            if (serial.available() >= 6 ) {
                _cin >> hex >> _status.length;
                if ( !_cin.fail() ) {
                    _cin.ignore(1, ' '); // skip whitespace
                    _rxstate = RX_PAYLOAD;
                } else {
                    _cin.ignore(serial.available(), '\n');
                    _rxstate = RX_INIT;
                }
            }
            break;

        case RX_PAYLOAD:
            if (serial.available() >= (int)_status.length ) {
                _status.ready = true;
            }
            break;

        case RX_ROUTE:
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
    _isWriting = true;
    for(uint8_t i = length; i != 0; i--) {
        serial.print( i2h(bytes[i-1] >> 4) );
        serial.print( i2h(bytes[i-1]     ) );
    }
}


void basic_crstream::writecmd(const char* const p_str, const uint16_t num, ...) {
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

    while (true) {
        #ifdef DEBUG_EN
            while (serial.available()) dbgSerial.print( (char) _cin.get() );
        #else
            _cin.ignore(serial.available());
        #endif

        _timems = millis();
        while ( !serial.available() and !_timeout() );
        if (_timeout()) break;
    }
    
    _isWriting = false;
}

bool basic_crstream::_timeout() {
    return millis() - _timems > TIMEOUTMS;
}
