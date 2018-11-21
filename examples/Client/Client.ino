#include <crstream.h>
#include <uniqueID.h>
#include "frame.h"

bool ledStatus = false;
uint32_t networkTime;

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
  #define CRESSON_TX_PIN  3
  #define CRESSON_RX_PIN  4
  SoftwareSerial            Serialx(CRESSON_TX_PIN, CRESSON_RX_PIN);
  crstream<SoftwareSerial>  cresson(Serialx /*, CRESSON_POWER_PIN*/);
#endif

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.destID    = 0x0000;      // default: 0xFFFF
  cresson.panID     = 0x8888;      // default: 0x0000
  cresson.autosleep = true;
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
  float analogA0 = 3.3*analogRead(A0)/1024;
  cresson << analogA0 << ledStatus;
  bool received = cresson.listen(3000);
  if (received) {
    digitalWrite(LED_BUILTIN, ledStatus);
    Serial.print(networkTime);
    Serial.println(ledStatus?F(" HIGH"):F(" LOW"));
  }
  delay(1000);
}

void cresson_onReceived() {
  res_frame rcvFrame;
  if ( cresson.available() == sizeof(rcvFrame) ) {
    cresson >> rcvFrame;
    ledStatus = rcvFrame.ledStatus;
    networkTime = rcvFrame.networkTime;
  }
}