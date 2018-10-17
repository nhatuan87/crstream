#include <crstream.h>
#include <uniqueID.h>

const PROGMEM char myString[]   = "And who are you, the proud lord said, that I must bow so low? Only a cat of a different coat, that's all the truth I know.\r\n"  ;

/*
 * Cresson RF module connects to uC using UART serial interface.
 * It supports HardwareSerial / SoftwareSerial and power pin.
 * Please modify instantiation below corresponding to actual hardware.
 */

//#define CRESSON_POWER_PIN   NO_POWERPIN

#if defined(HAVE_HWSERIAL1)
  crstream<>  cresson(Serial1 /*, CRESSON_POWER_PIN*/);
#else
  #include <SoftwareSerial.h>
  #define CRESSON_TX_PIN  3
  #define CRESSON_RX_PIN  4
  SoftwareSerial            Serial1(CRESSON_TX_PIN, CRESSON_RX_PIN);
  crstream<SoftwareSerial>  cresson(Serial1 /*, CRESSON_POWER_PIN*/);
#endif

void setup() {
  // serial debug
  Serial.begin(19200);
  pinMode(LED_BUILTIN, OUTPUT);

  // cresson setup
  #ifdef CRESSON_POWER_PIN
  cresson.powerOn();
  #endif

  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.panID     = 0x0123;
  cresson.autosleep = true;
  cresson.begin();
  while (!cresson.isAlive()) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void loop() {
  uint16_t i = 0;
  while (i<strlen_P(myString)) {
    char c = pgm_read_byte_near(myString + i);
    cresson << c;
    i++;
  }
  cresson.listen(); // actually send data, then sleep
  delay(5000);      // delay 5 seconds
}
