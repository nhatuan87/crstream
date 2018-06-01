#include <crstream.h>
#include <EEPROM.h>
#include "myframe.h"

// Cresson instance
#if defined(HAVE_HWSERIAL1)
  crstream<>  cresson(Serial1);
#else
  #include <SoftwareSerial.h>
  #define   CRESSON_RX      7
  #define   CRESSON_TX      8
  SoftwareSerial            Serial1(CRESSON_RX, CRESSON_TX);
  crstream<SoftwareSerial>  cresson(Serial1);
#endif

typedef struct {
    uint16_t    days;
    uint8_t     hours;
    uint8_t     mins;
    uint8_t     secs;
} mytime_t;

typedef struct {
  uint16_t  nodeID;
  myframe   payload;
} descriptor;

mytime_t    mytime;
uint64_t    sensor_presence;

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.mhmode    =  MHMASTER ; // default: MHSLAVE
  cresson.begin();

  Serial.println( F("The device is listening upto 63 sensor nodes which send its uptime and VCC."));
  Serial.println( F("A slot of 16 bytes EEPROM are reserved for a sensor, making totally 1024 bytes of EEPROM."));
  Serial.println( F("Slot 0 deditated for this Gateway, slot 1 for Sensor whose ID%64=1, etc.") );
  Serial.println( F("The device does NOT support sensor whose ID divisible by 64.") );
  Serial.println( F("Type '!': clear all recent sensors' value"));
  Serial.println( F("Type '?': print all recent sensors' value"));

  EEPROM.get(0, sensor_presence);
}

void loop() {
  descriptor  mydescriptor;
  if ( cresson.available() ) {
    mydescriptor.nodeID    = cresson.sender();
    cresson >> mydescriptor.payload;
    if ( ! cresson.status() ) {
      uint8_t descriptor_no  = mydescriptor.nodeID & 0x003F ;
      if (descriptor_no != 0) {
        descriptor_store(descriptor_no, mydescriptor);
        descriptor_print(descriptor_no);
      } else {
        Serial.println( F("Don't support Node whose ID % 64 = 0.") );
        Serial.print  ( F("nodeID = ")                     );
        Serial.println( mydescriptor.nodeID, HEX           );
      }
    } else {
      Serial.println( F("Frame error (hexascii format required)") );
    }
  }
  cresson.update();

  char c = (char)Serial.read();
  if (c == '!') {
    descriptor_clear();
    Serial.println(F("All descriptors cleared!"));
  } else if (c == '?') {
    Serial.println( F("-----------------------") );
    for(uint8_t i=1; i<64; i++) {
      if ( (sensor_presence >> i) & 0x01) descriptor_print(i);
    }
    Serial.println( F("-----------------------") );
  }
}

void descriptor_print(uint8_t descriptor_no) {
  uint16_t addr = descriptor_no << 4;
  descriptor  mydescriptor;
  EEPROM.get(addr, mydescriptor);
  Serial.print  ( F("*** SLOT #")              );
  Serial.println( descriptor_no );
  
  Serial.print  ( F("nodeID = ")                     );
  Serial.println( mydescriptor.nodeID, HEX           );
  
  Serial.print  ( F("uptime = ")              );
  mytime_print(mydescriptor.payload.uptime);

  Serial.print  ( F("power = ")               );
  Serial.println( mydescriptor.payload.value         );

  Serial.println();
}

void descriptor_store(uint8_t descriptor_no, descriptor& mydescriptor) {
  uint16_t addr = descriptor_no << 4;
  char*    obj  = (char*) &mydescriptor;
  for(uint8_t i=0; i<sizeof(mydescriptor); i++) {
    EEPROM.update(addr+i, obj[i]);
  }

  obj  = (char*) &sensor_presence;
  bitSet(obj[descriptor_no/8], descriptor_no%8);
  for(uint8_t i=0; i<sizeof(sensor_presence); i++) {
    EEPROM.update(i, obj[i]);
  }
}

void descriptor_clear() {
  for(uint8_t i=0; i<sizeof(sensor_presence); i++) {
    EEPROM.update(i, 0);
  }
  EEPROM.get(0, sensor_presence);
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
    Serial.print  ( mytime.days                        );
    Serial.print  ( F(" days ")                        );
    Serial.print  ( mytime.hours                       );
    Serial.print  ( F(" hours ")                       );
    Serial.print  ( mytime.mins                        );
    Serial.print  ( F(" mins ")                        );
    Serial.print  ( mytime.secs                        );
    Serial.println( F(" secs")                         );
}
