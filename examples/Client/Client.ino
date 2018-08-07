#include <crstream.h>
#include <uniqueID.h>
#include "frame.h"

#if defined(HAVE_HWSERIAL1)
  crstream<>  cresson(Serial1);
#else
  #include <SoftwareSerial.h>
  #define CRESSON_TX_PIN  3
  #define CRESSON_RX_PIN  4
  SoftwareSerial            Serial1(CRESSON_TX_PIN, CRESSON_RX_PIN);
  crstream<SoftwareSerial>  cresson(Serial1);
#endif

bool ledStatus = false;
uint32_t networkTime;

void setup() {
  // serial debug
  Serial.begin(19200);
  pinMode(LED_BUILTIN, OUTPUT);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.destID    = 0x0000;      // default: 0xFFFF
  cresson.panID     = 0x8888;      // default: 0x0000
  cresson.autosleep = true;
  cresson.begin();
}

void loop() {
  float analogA0 = 3.3*analogRead(A0)/1024;
  cresson << analogA0 << (uint8_t)ledStatus;
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
    ledStatus = (bool)rcvFrame.ledStatus;
    networkTime = rcvFrame.networkTime;
  }
}