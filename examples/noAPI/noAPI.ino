#include <crstream.h>
#include <uniqueID.h>

/*
 * Cresson RF module connects to uC using UART serial interface.
 * It supports HardwareSerial / SoftwareSerial and power pin.
 * Please modify instantiation below corresponding to actual hardware.
 * When Cresson connected successfully, the BUILT-IN LED will blink,
 * and you can use Serial Monitor to interact with Cresson.
 */

#define CRESSON_POWER_PIN   NO_POWERPIN

#if defined(ARDUINO_ARCH_AVR) && defined(HAVE_HWSERIAL1)
  crstream<>  cresson(Serial1, CRESSON_POWER_PIN);
#else
  #include <SoftwareSerial.h>
  #define CRESSON_TX_PIN  7
  #define CRESSON_RX_PIN  8
  SoftwareSerial            Serialx(CRESSON_TX_PIN, CRESSON_RX_PIN);
  crstream<SoftwareSerial>  cresson(Serialx, CRESSON_POWER_PIN);
#endif

uint32_t localtime;
bool cresson_isAlive;

void setup() {
  // serial debug
  if (&cresson.serial != &Serial) Serial.begin(19200);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  localtime = millis();
  cresson_isAlive = cresson.isAlive();
}

void loop() {
  if (cresson_isAlive and (millis() - localtime) >= 1000) {
    localtime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

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