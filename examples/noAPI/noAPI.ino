#include <crstream.h>
#include <uniqueID.h>

#if defined(HAVE_HWSERIAL1)
  crstream<>  cresson(Serial1);
#else
  #include <SoftwareSerial.h>
  #define CRESSON_TX_PIN  3
  #define CRESSON_RX_PIN  4
  SoftwareSerial            Serial1(CRESSON_TX_PIN, CRESSON_RX_PIN);
  crstream<SoftwareSerial>  cresson(Serial1);
#endif

void setup() {
  // serial debug
  Serial.begin(19200);
  pinMode(LED_BUILTIN, OUTPUT);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.destID    = BROADCAST_ID;// default: 0xFFFF
  cresson.panID     = 0x0123;      // default: 0x0000
  cresson.channel   = 33;          // default: 33
  cresson.datarate  = DATA_50K;    // default: DATA_50K
  cresson.mhmode    = MHSLAVE;     // default: MHSLAVE
  cresson.baudmode  = BAUD_9600;   // default: BAUD_9600
  cresson.begin();
  while (!cresson.isAlive()) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void loop() {
  // Cresson --> Serial
  while(cresson.serial.available()) {
    char c = (char) cresson.serial.read();
    Serial.write(c);
  }
  // Serial --> Cresson
  while( Serial.available() ) {
    char c = (char) Serial.read();
    cresson.serial.write(c);
  }
}