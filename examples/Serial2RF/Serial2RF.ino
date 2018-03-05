#include <crstream.h>
#include <EEPROM.h>
#include "board_CressonMote.h"
const PROGMEM char P_date[]   = __DATE__  ;
const PROGMEM char P_time[]   = __TIME__  ;

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.baudmode  =    B_9600 ; // default: 9600 bps
  cresson.selfID    = randomID(P_date, P_time) ; // default: 0x0000
  cresson.destID    =    0xFFFF ; // default: 0xFFFF
  cresson.panID     =         0 ; // default: 0x0000
  cresson.datarate  =     D_10K ; // default: 10 kbps
  cresson.channel   =        33 ; // default: 33
  cresson.mhmode    =  MHSLAVE  ; // default: MHSLAVE
  cresson.autosleep =   false   ;
  cresson.begin();

}

void loop() {
  while ( cresson.available() ) {
    char rx;
    cresson >> rx;
    Serial.write(rx);
  }
  cresson.update();
}

void serialEvent() {
  uint32_t timems = millis();
  int count = 0;
  while ( millis() - timems < 10 and count < 30) {
    if ( Serial.available() ) {
      char tx = (char) Serial.read();
      cresson << tx;
      timems = millis();
      count ++;
    }
  }
}

