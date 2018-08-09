#include <crstream.h>
#include <uniqueID.h>

// Cresson instance
#if defined(HAVE_HWSERIAL1)
  crstream<>  cresson(Serial1);
#else
  #include <SoftwareSerial.h>
  #define   CRESSON_TX_PIN      3
  #define   CRESSON_RX_PIN      4
  SoftwareSerial            Serial1(CRESSON_TX_PIN, CRESSON_RX_PIN);
  crstream<SoftwareSerial>  cresson(Serial1);
#endif

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.panID     = 0x0123;
  cresson.begin();
}

void loop() {
  cresson.listen();
}

// RF --> serial
void cresson_onReceiving() {
  while ( cresson.available() ) {
    char rx = 0;
    cresson >> rx;
    Serial.write(rx);
  }
}

// serial --> RF
void serialEvent() {
  while ( Serial.available() ) {
    char tx = (char) Serial.read();
    cresson << tx;
  }
}