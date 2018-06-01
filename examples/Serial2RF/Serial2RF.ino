#include <crstream.h>

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

const PROGMEM char P_date[]   = __DATE__  ;
const PROGMEM char P_time[]   = __TIME__  ;

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.selfID    = randomID(P_date, P_time) ; // default: 0x0000
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


