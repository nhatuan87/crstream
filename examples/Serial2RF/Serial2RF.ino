#include <crstream.h>
#include <uniqueID.h>

/*
 * Cresson RF module connects to uC using UART serial interface.
 * It supports HardwareSerial / SoftwareSerial and power pin.
 * Please modify instantiation below corresponding to actual hardware.
 */

//#define CRESSON_POWER_PIN   NO_POWERPIN

#if defined(ARDUINO_ARCH_AVR) && defined(HAVE_HWSERIAL1)
  crstream<>  cresson(Serial1 /*, CRESSON_POWER_PIN*/);
#else
  #include <SoftwareSerial.h>
  #define CRESSON_TX_PIN  7
  #define CRESSON_RX_PIN  8
  SoftwareSerial            Serialx(CRESSON_TX_PIN, CRESSON_RX_PIN);
  crstream<SoftwareSerial>  cresson(Serialx /*, CRESSON_POWER_PIN*/);
#endif

bool cresson_isAlive;
uint32_t localtime;

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.panID     = 0x0123;
  cresson.begin();

  while(!cresson.isAlive()) {
    delay(1000);
  }

  pinMode(LED_BUILTIN, OUTPUT);
  localtime = millis();
}

void loop() {
  if (millis() - localtime >= 1000) {
    localtime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
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