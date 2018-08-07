/*
 * Copyright (C) 2018 CME Vietnam Co. Ltd.
 * v0.6.2 - Tuan Tran
*/
#ifndef CRSTREAM_H
#define CRSTREAM_H

#include <Arduino.h>

#define BAUDMODE_NUM      7
#define BROADCAST_ID      0xFFFF

// Valid data rate
typedef enum {
    DATA_200K     = 0,
    DATA_100K     = 1,
    DATA_50K      = 2,
    DATA_10K      = 3
} DATARATE;

// Valid baud rate
typedef enum {
    BAUD_9600     = 1,
    BAUD_19200    = 2,
    BAUD_38400    = 3,
    BAUD_57600    = 4,
    BAUD_115200   = 5,
    BAUD_230400   = 6,
    BAUD_460800   = 7
} BAUDMODE;

typedef enum {
    P2P        = 0,
    MHMASTER   = 1,
    MHREPEATER = 2,
    MHSLAVE    = 3
} MHMODE;

typedef enum {
    stIDLE              = 0,
    stWAIT_FOR_IDLE     = 1,
    stSLEEP             = 2,
    stRX_HEADER         = 3,
    stRX_SENDER         = 4,
    stRX_DEST           = 5,
    stRX_MSGID          = 6,
    stRX_RSSI           = 7,
    stRX_LENGTH         = 8,
    stRX_PAYLOAD        = 9,
    stTX_HEADER_MHSEND  = 10,
    stTX_HEADER_MHACK   = 11,
    stTX_RESPOND        = 12,
} RXSTATE;

typedef enum {
    stTX_IDLE       ,
    stTX_WRITE_DATA
} TXSTATE;

typedef enum {
    TEXT_MODE,
    BIN_MODE
} DATA_MODE;

extern const PROGMEM char P_listen[]   ;
extern const PROGMEM char P_baud[]     ;
extern const PROGMEM char P_sleep[]    ;
extern const PROGMEM char P_reset[]    ;
extern const PROGMEM char P_mhrtreq[]  ;
extern const PROGMEM char P_send[]     ;
extern const PROGMEM char P_sendto[]   ;
extern const PROGMEM char P_sysreg[]   ;
extern const PROGMEM char P_mhdata[]   ;
extern const PROGMEM char P_mhsend[]   ;
extern const PROGMEM char P_mhack[]    ;
extern const PROGMEM uint32_t   _baud[BAUDMODE_NUM];

typedef struct {
        uint16_t            sender;
        uint16_t            destination;
        uint16_t            msgid;
        int16_t             rssi;
        uint16_t            length;
        uint16_t            available;
        int16_t             sendAck;
        bool                received;
        bool                decodeFailed;
        bool 				alive;
} status_t;

class basic_crstream {
    public:
        basic_crstream(Stream& serial);
        uint16_t                selfID      ;
        uint16_t                destID      ;
        uint16_t                panID       ;
        uint8_t                 baudmode    ;
        uint8_t                 datarate    ;
        uint8_t                 channel     ;
        uint8_t                 mhmode      ;
        bool                    autosleep   ;
        bool                    datamode    ;
        Stream&                 serial      ;

        template<typename T> basic_crstream& operator<< (T payload);
        template<typename T> basic_crstream& operator>> (T& payload);
        virtual bool            begin() {return true;}
        bool                    decodeFailed()  ;
        uint16_t                sender()        ;
        uint16_t                destination()   ;
        uint16_t                length()        ;
        uint16_t                available()     ;
        int16_t                 rssi()          ;
        void                    onReceivedCallback    (void (*fnc)(void));
        void                    onReceivingCallback   (void (*fnc)(void));
        void                    sendFailedCallback    (void (*fnc)(void));
        void                    wiringErrorCallback   (void (*fnc)(void));
        void                    sleep()         ;
        void                    wakeup()        ;
        bool                    listen(uint32_t timems=0);
        void                    writecmd(const char* const p_str, const uint16_t num, ...);
        void                    execute()         ;
        bool                    isAlive()       ;
        // static functions
        static int              asc2hex (char c     );
        static uint8_t          bin2bcd (uint8_t val);
        static char             i2h(uint8_t i);

    private:
        void                    (*_onReceivedFnc)(void);
        void                    (*_onReceivingFnc)(void);
        void                    (*_sendFailedFnc)(void);
        void                    (*_wiringErrorFnc)(void);
        uint8_t                 _currentState;
        uint8_t                 _txState     ;
        uint32_t                _timems      ;
        status_t                _result      ;
        uint16_t                _chrnum      ;
        static const char       _delim = ',' ;
        char                    _tempbuf[7]  ;

        void                    _update()    ;
        int                     _getchr()    ;
        int                     _getint()    ;
        bool                    _timeout(uint32_t timeoutms=100)   ;
        void                    _clear()     ;
        void                    _write(const char* bytes, const uint8_t length);
        void                    _switchSM(uint8_t newState);
        bool                    _listen(uint32_t timems=100);
        bool                    _headerMatched(const char* const p_str);
};

template<typename T> basic_crstream& basic_crstream::operator<< (T payload) {
    if (_txState == stTX_IDLE) {
        writecmd(P_sendto, 1, destID);
        serial.print(_delim);
        _txState = stTX_WRITE_DATA;
    }
    _write((char*) &payload, sizeof(T) );
    return *this;
}

template<typename T> basic_crstream& basic_crstream::operator>> (T& payload){
    if ( this->available() >= sizeof(T) ) {
        char* s = (char*) &payload;
        // s[0] = payload's LSB
        for (uint8_t i = sizeof(T); i != 0; i--) {
            int retval = _getchr();
            _result.decodeFailed |= (retval == -1);
            s[i-1] = (char)retval;
        }
        _chrnum += sizeof(T);
    } else {
        _result.decodeFailed = true;
    }
    return *this;
}

template<class Tserial=HardwareSerial>
class crstream : public basic_crstream {
    public:
        crstream(Tserial& serial) : basic_crstream(serial){};
        bool    begin();
};

template<class Tserial>
bool crstream<Tserial>::begin() {
    if (baudmode == 0 or baudmode > BAUDMODE_NUM) baudmode = BAUD_9600; // baudmode = 1~BAUDMODE_NUM
    if (datarate >  3                           ) datarate = DATA_50K ;

    Tserial* port = (Tserial*) &serial;
    for (uint8_t i = 1; i <= BAUDMODE_NUM; i++) {
        if (i == baudmode) continue;

        port->begin( pgm_read_dword_near(_baud + i - 1) );
        execute();   // dummy command
        writecmd(P_baud, 1, baudmode);
        execute();
    }

    port->begin( pgm_read_dword_near(_baud + baudmode - 1) );
    execute();   // dummy command

    writecmd(P_sysreg, 2, 0x01, selfID            ); execute();
    //writecmd(P_sysreg, 2, 0x02, destID            ); execute();
    writecmd(P_sysreg, 2, 0x03, panID             ); execute();
    writecmd(P_sysreg, 2, 0x04, bin2bcd(channel)  ); execute();
    writecmd(P_sysreg, 2, 0x05, datarate          ); execute();
    writecmd(P_sysreg, 2, 0x06, 1                 ); execute();    // Clear Channel Assessment
    writecmd(P_sysreg, 2, 0x08, 0                 ); execute();    // TxRetry
    writecmd(P_sysreg, 2, 0x0B, 1                 ); execute();    // Wake by Uart
    writecmd(P_sysreg, 2, 0x30, mhmode            ); execute();
    if (mhmode == MHREPEATER) {
        writecmd(P_mhrtreq, 0);
        execute();
    }
    listen();
    return true;
}

#endif // CRESSONSTREAM_H