#include <crstream.h>
#include <EEPROM.h>
#include "board_CressonMote.h"
#include "myframe.h"

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
  Serial.begin(9600);

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

  Serial.println( F("The device is listening to maximum 63 sensor nodes."));
  Serial.println( F("The most recent sensor values, including its power & uptime, would be stored in a EEPROM slot."));
  Serial.println( F("Every nodes take a slot numberred according to the last 6-bit of its NodeID.") );
  Serial.println( F("The device does NOT support sensor node whose ID divisible by 64 (Eg. nodeID=0x12C0 )") );
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
  bitSet(sensor_presence, descriptor_no);
  for(uint8_t i=0; i<sizeof(sensor_presence); i++) {
    EEPROM.update(i, obj[i]);
  }
}

void descriptor_clear() {
  for(uint8_t i=0; i<sizeof(sensor_presence); i++) {
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
    Serial.print  ( mytime.days                        );
    Serial.print  ( F(" days ")                        );
    Serial.print  ( mytime.hours                       );
    Serial.print  ( F(" hours ")                       );
    Serial.print  ( mytime.mins                        );
    Serial.print  ( F(" mins ")                        );
    Serial.print  ( mytime.secs                        );
    Serial.println( F(" secs")                         );
}
