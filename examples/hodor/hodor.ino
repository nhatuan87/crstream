#include <crstream.h>
#include <MyHwAVR.h>
#include "myframe.h"
#include "board_CressonMote.h"

#define SECOND  1000UL  // millisecond
#define MINUTE    60    // seconds
#define HOUR    3600    // seconds

const uint16_t  sampleInterval = 15*MINUTE; // Max = 18.2*HOUR
const PROGMEM char P_date[]   = __DATE__  ;
const PROGMEM char P_time[]   = __TIME__  ;

uint32_t    sample_no = 0;
myframe     payload;

void setup() {
  // cresson setup
  cresson.baudmode  =    B_9600 ; // default: 9600 bps
  cresson.selfID    = randomID(P_date, P_time) ; // default: 0x0000
  cresson.destID    =         0 ; // default: 0xFFFF
  cresson.panID     =         0 ; // default: 0x0000
  cresson.datarate  =     D_10K ; // default: 10 kbps
  cresson.channel   =        33 ; // default: 33
  cresson.mhmode    =   MHSLAVE ; // default: MHSLAVE
  cresson.autosleep =   true    ;
  cresson.begin();
}

void loop() {

  payload.uptime    = sample_no++ * sampleInterval; // seconds
  payload.value     = sensorRead();
  cresson << payload;

  cresson.update();
  hwSleep(sampleInterval*SECOND);
}

// read AVCC
float sensorRead() {
    return (float) hwCPUVoltage()/1000; // volt
}
