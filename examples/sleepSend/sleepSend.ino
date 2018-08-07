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

const PROGMEM char myString[]   = "And who are you, the proud lord said, that I must bow so low? Only a cat of a different coat, that's all the truth I know.\r\n"  ;
void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.panID     = 0x0123;
  cresson.autosleep = true;
  cresson.begin();
}

void loop() {
  uint16_t i = 0;
  while (i<strlen_P(myString)) {
    char c = pgm_read_byte_near(myString + i);
    cresson << c;
    i++;
  }
  cresson.listen(); // actually send data
  delay(5000);
}
