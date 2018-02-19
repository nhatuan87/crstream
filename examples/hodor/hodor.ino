#include <crstream.h>
#include <MyHwAVR.h>
//#include <RTClib.h>

#define minute        60000UL // milliseconds
#define SAMPLEPERIOD  15      // minutes
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

uint16_t sample_no = 0;

void setup() {
  // seed timestamp into random function
  //DateTime  compiletime(F(__DATE__), F(__TIME__));
  //randomSeed(compiletime.unixtime());

  // serial debug
  dbgSerial.begin(9600);

  // cresson setup
  cresson.baudmode  =    B_9600 ; // default: 9600 bps
  cresson.selfID    = (uint16_t)random(0x0001, 0xFFFE); // default: 0x0000
  cresson.destID    =         0 ; // default: 0xFFFF
  cresson.panID     =         0 ; // default: 0x0000
  cresson.datarate  =     D_10K ; // default: 10 kbps
  cresson.channel   =        33 ; // default: 33
  cresson.mhmode    =   MHSLAVE ; // default: MHSLAVE
  cresson.autosleep =   true    ;
  cresson.begin();
}

void loop() {

  // read AVCC
  float AVCCVoltage = (float) hwCPUVoltage()/1000;
  float uptime      = (++sample_no) * (float)SAMPLEPERIOD/60; // hours
  cresson << uptime << AVCCVoltage;

  while( !cresson.isSleeping() ) cresson.update();
  hwSleep(SAMPLEPERIOD*minute);
}
