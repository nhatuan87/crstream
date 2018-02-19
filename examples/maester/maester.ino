#include <crstream.h>
#include <EEPROM.h>
#include "myframe.h"

//#define TEST

#ifdef TEST
crstream<> cresson(Serial);
#else
#if defined (__AVR_ATmega328P__)
  #include <SoftwareSerial.h>
  SoftwareSerial            Serial1(3, 4);
  crstream<SoftwareSerial>  cresson(Serial1);
#elif defined (__AVR_ATmega2560__)
  crstream<>                cresson(Serial1);
#endif
#endif // TEST

typedef struct {
    uint32_t    days;
    uint8_t     hours;
    uint8_t     mins;
} mytime_t;

typedef struct {
  uint16_t  nodeID;
  uint32_t  max_uptime;
  myframe   payload;
} descriptor;

descriptor  mydescriptor;
mytime_t    mytime;

void setup() {
  // serial debug
  dbgSerial.begin(9600);

  EEPROM.get(0, mydescriptor);
  descriptor_print();

  // cresson setup
  cresson.baudmode  =    B_9600 ; // default: 9600 bps
  cresson.selfID    =         0 ; // default: 0x0000
  cresson.destID    =    0xFFFF ; // default: 0xFFFF
  cresson.panID     =         0 ; // default: 0x0000
  cresson.datarate  =     D_10K ; // default: 10 kbps
  cresson.channel   =        33 ; // default: 33
  cresson.mhmode    =  MHMASTER ; // default: MHSLAVE
  cresson.autosleep =   false   ;
  cresson.begin();

}

void loop() {
  if ( cresson.available() ) {
    cresson >> mydescriptor.payload;
    mydescriptor.max_uptime = max(mydescriptor.max_uptime, mydescriptor.payload.uptime);
    mydescriptor.nodeID     = cresson.sender();
    if ( ! cresson.status() ) {
      descriptor_store(0, (char*) &mydescriptor, sizeof(mydescriptor));
      descriptor_print();
    } else {
      dbgSerial.println( F("Frame error (hexascii format required)") );
    }
    cresson.clear();
  }
  cresson.update();
}

void descriptor_print() {
  dbgSerial.println( F("--DESCRIPTOR----")              );
  
  dbgSerial.print  ( F("nodeID = ")                     );
  dbgSerial.println( mydescriptor.nodeID, HEX           );
  
  dbgSerial.print  ( F("max uptime = ")                 );
  mytime_print(mydescriptor.max_uptime);
  
  dbgSerial.print  ( F("recent uptime = ")              );
  mytime_print(mydescriptor.payload.uptime);

  dbgSerial.print  ( F("recent value = ")               );
  dbgSerial.println( mydescriptor.payload.value         );

  dbgSerial.println( F("----------------")     );
}

void descriptor_store(uint16_t address, const char* obj, const uint8_t size) {
  for(uint8_t i=0; i<size; i++) {
    EEPROM.update(address+i, obj[i]);
  }
}

mytime_t& conv(uint32_t& uptime) {
    mytime.mins     =  uptime % 60;
    mytime.hours    = (uptime / 60) % 24;
    mytime.days     = (uptime / 60) / 24;
    return mytime;
}

void mytime_print(uint32_t& uptime) {
    conv(uptime);
    dbgSerial.print  ( mytime.days                        );
    dbgSerial.print  ( F(" days ")                        );
    dbgSerial.print  ( mytime.hours                       );
    dbgSerial.print  ( F(" hours ")                       );
    dbgSerial.print  ( mytime.mins                        );
    dbgSerial.println( F(" mins")                         );
}