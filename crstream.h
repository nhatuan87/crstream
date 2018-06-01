/*
 * Copyright (C) 2018 CME Vietnam Co. Ltd.
 * v0.2 - Tuan Tran
*/
#ifndef CRSTREAM_H
#define CRSTREAM_H

#include <Arduino.h>

#include <StandardCplusplus.h>
#include <serstream>

using namespace std;

#define BAUDMODE_NUM      7
#define DEBUG_EN

// Valid data rate
typedef enum {
    D_200K     = 0,
    D_100K     = 1,
    D_50K      = 2,
    D_10K      = 3
} DATARATE;

// Valid baud rate
typedef enum {
    B_9600     = 1,
    B_19200    = 2,
    B_38400    = 3,
    B_57600    = 4,
    B_115200   = 5,
    B_230400   = 6,
    B_460800   = 7
} BAUDMODE;

typedef enum {
    P2P        = 0,
    MHMASTER   = 1,
    MHREPEATER = 2,
    MHSLAVE    = 3
} MHMODE;

typedef enum {
    RX_INIT     ,
    RX_SENDER   ,
    RX_DEST     ,
    RX_ID       ,
    RX_RSSI     ,
    RX_LENGTH   ,
    RX_PAYLOAD  ,
    RX_ROUTE
} RXSTATE;

// Global functions & variables
int        asc2hex (char c     );
uint8_t    bin2bcd (uint8_t val);
char       i2h(uint8_t i);

extern const PROGMEM char P_listen[]   ;
extern const PROGMEM char P_baud[]     ;
extern const PROGMEM char P_sleep[]    ;
extern const PROGMEM char P_reset[]    ;
extern const PROGMEM char P_mhrtreq[]  ;
extern const PROGMEM char P_send[]     ;
extern const PROGMEM char P_sendto[]   ;
extern const PROGMEM char P_sysreg[]   ;
extern const PROGMEM char P_mhdata[]   ;

extern char             _tempbuf[7];
extern const uint32_t   _baud[BAUDMODE_NUM] PROGMEM;
extern const char       _delim;

uint16_t crc16_update(uint16_t& crc, const char* const p_str);
uint16_t randomID(const char* const p_date, const char* const p_time);

class Status {
    public:
        Status()            { clear(); }
        uint16_t            sender;
        uint16_t            destination;
        uint16_t            id;
        int16_t             rssi;
        uint16_t            length;
        uint16_t            available;
        uint8_t             rxstate;
        bool                fail;
        void                clear();
};

class basic_crstream {
    public:
        basic_crstream(Stream& serial, uint32_t timeoutms);
        uint16_t                selfID      ;
        uint16_t                destID      ;
        uint16_t                panID       ;
        uint8_t                 baudmode    ;
        uint8_t                 datarate    ;
        uint8_t                 channel     ;
        uint8_t                 mhmode      ;
        bool                    autosleep   ;
        Stream&                 serial      ;

        virtual void            begin(){};
        void                    update();
        bool                    status()        {return _status.fail;}
        uint16_t                available()     {_status.available = min(_status.length/2 - _getnum, (uint16_t)serial.available()/2); return _status.available;}
        uint16_t                sender()        {return _status.sender;}
        void                    flush()     ;
        void                    writecmd(const char* const p_str, const uint16_t num, ...);

        template<typename T> basic_crstream& operator<< (T payload);
        template<typename T> basic_crstream& operator>> (T& payload);

    private:
        const uint32_t          _timeoutms   ;
        bool                    _isSleeping  ;
        bool                    _isWriting   ;
        uint32_t                _timems      ;
        Status                  _status      ;
        uint8_t                 _laststate;
        uint8_t                 _getnum;
        basic_iserialstream<char, char_traits<char>, Stream> _cin;
        
        void                    _clear()         { _cin.clear(); _clearbuf(); _status.clear(); _getnum = 0;}
        int                     _get()       ;
        void                    _rxProcess() ;
        bool                    _timeout()   ;
        void                    _clearbuf()  {_cin.ignore(serial.available(), '\n');}

        void                    _write(const char* bytes, const uint8_t length);
};

template<typename T> basic_crstream& basic_crstream::operator<< (T payload) {
    if (! _isWriting) {
        writecmd(P_sendto, 1, destID);
        serial.print(_delim);
    }
    _write((char*) &payload, sizeof(T) );
    return *this;
}

template<typename T> basic_crstream& basic_crstream::operator>> (T& payload){
    if ( this->available() >= sizeof(T) ) {
        char* s = (char*) &payload;
        // s[0] = payload's LSB
        for (uint8_t i = sizeof(T); i != 0; i--) {
            int retval = _get();
            _status.fail |= (retval == -1);
            s[i-1] = (char)retval;
        }
        _getnum += sizeof(T);
    } else {
        _status.fail = true;
    }
    return *this;
}

template<class Tserial=HardwareSerial>
class crstream : public basic_crstream {
    public:
        crstream(Tserial& serial, uint32_t timeoutms=100) : basic_crstream(serial, timeoutms){};
        void    begin();
};

template<class Tserial>
void crstream<Tserial>::begin() {
    if (baudmode == 0 or baudmode > BAUDMODE_NUM) baudmode = B_9600; // baudmode = 1~BAUDMODE_NUM
    if (datarate >  3                           ) datarate = D_50K ;

    Tserial* port = (Tserial*) &serial;
    for (uint8_t i = 1; i <= BAUDMODE_NUM; i++) {
        if (i == baudmode) continue;

        port->begin( pgm_read_dword_near(_baud + i - 1) );
        flush();   // dummy command
        writecmd(P_baud, 1, baudmode);
        flush();
    }

    port->begin( pgm_read_dword_near(_baud + baudmode - 1) );
    flush();   // dummy command

    writecmd(P_sysreg, 2, 0x01, selfID            ); flush();
    writecmd(P_sysreg, 2, 0x02, destID            ); flush();
    writecmd(P_sysreg, 2, 0x03, panID             ); flush();
    writecmd(P_sysreg, 2, 0x04, bin2bcd(channel)  ); flush();
    writecmd(P_sysreg, 2, 0x05, datarate          ); flush();
    writecmd(P_sysreg, 2, 0x06, 1                 ); flush();    // Clear Channel Assessment
    writecmd(P_sysreg, 2, 0x08, 0                 ); flush();    // TxRetry
    writecmd(P_sysreg, 2, 0x0B, 1                 ); flush();    // Wake by Uart
    writecmd(P_sysreg, 2, 0x30, mhmode            ); flush();
    if (mhmode == MHREPEATER) {
        writecmd(P_mhrtreq, 0);
        flush();
    }
}

#endif // CRESSONSTREAM_H