#include <crstream.h>
#include <EEPROM.h>
#include <MinimumSerial.h>

#include "myframe.h"
MinimumSerial dbgSerial;

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
    uint16_t    days;
    uint8_t     hours;
    uint8_t     mins;
    uint8_t     secs;
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

  Serial.println( F("The device is listening to sensor node's power voltage and its uptime."));
  Serial.println( F("The information would be stored in a descriptor."));
  Serial.println( F("To clear the descriptor, type '!'."));
  
  EEPROM.get(0, mydescriptor);
  descriptor_print();
}

void loop() {
  if ( cresson.available() ) {
    cresson >> mydescriptor.payload;
    mydescriptor.max_uptime = max(mydescriptor.max_uptime, mydescriptor.payload.uptime);
    
    mydescriptor.nodeID     = cresson.sender();
    if ( ! cresson.status() ) {
      descriptor_store();
      descriptor_print();
    } else {
      dbgSerial.println( F("Frame error (hexascii format required)") );
    }
  }
  cresson.update();

  char c = (char)dbgSerial.read();
  if (c == '!') {
    descriptor_clear();
    dbgSerial.println(F("The descriptor cleared!"));
  }
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

void descriptor_store() {
  char* obj = (char*) &mydescriptor;
  for(uint8_t i=0; i<sizeof(mydescriptor); i++) {
    EEPROM.update(i, obj[i]);
  }
}

void descriptor_clear() {
  char* obj = (char*) &mydescriptor;
  for(uint8_t i=0; i<sizeof(mydescriptor); i++) {
    EEPROM.update(i, 0);
  }
}

mytime_t& conv(uint32_t uptime) {
    mytime.days     =  uptime / 86400;
    mytime.hours    = (uptime % 86400) / 3600;
    mytime.mins     = (uptime % 3600 ) / 60;
    mytime.secs     =  uptime % 60;
    return mytime;
}

void mytime_print(uint32_t uptime) {
    conv(uptime);
    dbgSerial.print  ( mytime.days                        );
    dbgSerial.print  ( F(" days ")                        );
    dbgSerial.print  ( mytime.hours                       );
    dbgSerial.print  ( F(" hours ")                       );
    dbgSerial.print  ( mytime.mins                        );
    dbgSerial.print  ( F(" mins ")                        );
    dbgSerial.print  ( mytime.secs                        );
    dbgSerial.println( F(" secs")                         );
}
