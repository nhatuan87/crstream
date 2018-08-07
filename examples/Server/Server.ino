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

res_frame serverFrame;

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.panID     = 0x8888;      // default: 0x0000
  cresson.begin();
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
    bool ledStatus = !(bool)rcvFrame.ledStatus;
    sendFrame.networkTime = millis()/1000;
    sendFrame.ledStatus = (uint8_t)ledStatus;
    cresson << sendFrame;
    Serial.print(sendFrame.networkTime);
    Serial.print(F(" ["));
    Serial.print(cresson.sender());
    Serial.print(F("] "));
    Serial.print(rcvFrame.analogA0);
    Serial.println(ledStatus?F(" HIGH"):F(" LOW"));
  }
}