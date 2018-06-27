#include <crstream.h>
#include <uniqueID.h>

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

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.begin();
}

void loop() {
  // serial --> cresson
  while ( cresson.available() ) {
    char rx;
    cresson >> rx;
    Serial.write(rx);
  }
  
  // cresson --> serial (upto 16 bytes per frame)
  if ( Serial.available() ) {
    uint16_t txcnt = 0;
    uint32_t timems = millis();
    while ( millis() - timems < 10 and txcnt < 16) {
      if ( Serial.available() ) {
        char tx = (char) Serial.read();
        cresson << tx;
        timems = millis();
        txcnt++;
      } else {
        // wait for timeout
        continue;
      }
    }
  }
  cresson.update();
}