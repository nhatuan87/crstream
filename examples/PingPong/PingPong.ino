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

uint32_t localtime;
uint32_t p = 0;
uint16_t failcnt = 0;
uint16_t sendcnt = 0;
uint16_t rcvcnt = 0;

void setup() {
  // serial debug
  Serial.begin(19200);

  // cresson setup
  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.panID     = 0xABCD;
  cresson.begin();
}

void loop() {
  bool msgReceived = cresson.listen(3000);
  if (!msgReceived) {
    report();
    restart();
  }
  if (sendcnt%100 == 0) report();
}

void restart() {
    p = 0;
    sendcnt = 0;
    failcnt = 0;
    rcvcnt = 0;
    cresson << p;
    sendcnt++;
    localtime = millis();
}

void report() {
    Serial.print(F("Sent:")); Serial.print(sendcnt);
    Serial.print(F(" Failed:")); Serial.print(failcnt);
    Serial.print(F(" Received:")); Serial.print(rcvcnt);
    Serial.print(F(" TOF(ms):")); Serial.print((float)(millis() - localtime)/(sendcnt + rcvcnt));
    Serial.println();  
}

void cresson_onReceived() {
  cresson.destID = cresson.sender();
  if ( cresson.available() == sizeof(p)) {
    cresson >> p;
    Serial.println(p);
    rcvcnt++;
    cresson << ++p;
    sendcnt++;
  }
}

void cresson_wiringError() {
  Serial.println(F("Error: Please check Cresson wiring"));
}

void cresson_sendFailed() {
  failcnt++;
  Serial.println(F("Send Failed"));
}