#include <crstream.h>
#include <uniqueID.h>
#include "frame.h"

res_frame serverFrame;

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
  cresson.panID     = 0x8888;      // default: 0x0000
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
  cresson.listen();
}

void cresson_onReceived() {
  cresson.destID = cresson.sender();
  req_frame rcvFrame;
  res_frame sendFrame;
  if (cresson.available() == sizeof(rcvFrame) ) {
    cresson >> rcvFrame.analogA0 >> rcvFrame.ledStatus;
    bool ledStatus = !rcvFrame.ledStatus;
    sendFrame.networkTime = millis()/1000;
    sendFrame.ledStatus = ledStatus;
    cresson << sendFrame;
    Serial.print(sendFrame.networkTime);
    Serial.print(F(" ["));
    Serial.print(cresson.sender());
    Serial.print(F("] "));
    Serial.print(rcvFrame.analogA0);
    Serial.println(ledStatus?F(" HIGH"):F(" LOW"));
  }
}