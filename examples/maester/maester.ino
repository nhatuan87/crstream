#include <crstream.h>
#include <EEPROM.h>

#define minute        60000UL // milliseconds
//#define TEST

#ifdef TEST
crstream<> cresson(Serial);
#else
#if defined (__AVR_ATmega328P__)
  #include <SoftwareSerial.h>
  SoftwareSerial            Serial1(3, 4);
  crstream<SoftwareSerial>  cresson(Serial1);
#elif defined (__AVR_ATmega2560__)
  crstream<>                  cresson(Serial1);
#endif
#endif // TEST

typedef struct {
  float     uptime;
  float     value;
} myframe;

typedef struct {
  uint16_t  sender;
  float     max_uptime;
  myframe   payload;
} descriptor;

descriptor mydescriptor;

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
  // request & respond example
  if ( cresson.available() ) {
    cresson >> mydescriptor.payload;
    mydescriptor.max_uptime = max(mydescriptor.max_uptime, mydescriptor.payload.uptime);
    mydescriptor.sender     = cresson.sender();
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
  dbgSerial.println( F("--DESCRIPTOR----")     );
  
  dbgSerial.print  ( F("sender (nodeID)=")               );
  dbgSerial.println( mydescriptor.sender,HEX             );
  dbgSerial.print  ( F("max uptime (hours)=")            );
  dbgSerial.println( mydescriptor.max_uptime             );
  dbgSerial.print  ( F("recent uptime (hours)=")         );
  dbgSerial.println( mydescriptor.payload.uptime         );
  dbgSerial.print  ( F("recent value (volt)=")           );
  dbgSerial.println( mydescriptor.payload.value          );

  dbgSerial.println( F("----------------")     );
}

void descriptor_store(uint16_t address, const char* obj, const uint8_t size) {
  for(uint8_t i=0; i<size; i++) {
    EEPROM.update(address+i, obj[i]);
  }
}

