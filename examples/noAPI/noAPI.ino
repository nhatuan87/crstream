#include <crstream.h>
#include <uniqueID.h>

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
  if (&cresson.serial != &Serial) Serial.begin(19200);

  // cresson setup
  #ifdef CRESSON_POWER_PIN
  cresson.powerOn();
  #endif
  
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  while (!cresson.isAlive()) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void loop() {
  if (&cresson.serial != &Serial) {
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
}