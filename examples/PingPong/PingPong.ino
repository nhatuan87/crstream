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

uint32_t localtime;
uint32_t p = 0;
uint16_t TXCnt        = 0;
uint16_t RXCnt        = 0;
uint16_t TXFailedCnt  = 0;
uint16_t RXFailedCnt  = 0;

void setup() {
  // serial debug
  Serial.begin(19200);
  pinMode(LED_BUILTIN, OUTPUT);

  // cresson setup
  #ifdef CRESSON_POWER_PIN
  cresson.powerOn();
  #endif

  cresson.selfID    = uniqueID() ; // default: 0x0000
  cresson.panID     = 0xABCD;
  cresson.begin();
  while (!cresson.isAlive()) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  localtime = millis();
}

void loop() {
  bool msgReceived = cresson.listen(3000);
  if (!msgReceived) {
    if (TXCnt != 0 and RXCnt != 0) {
      RXFailedCnt ++;
      report();
    }
    p = 0;
    cresson << p;
    TXCnt++;
  }
  if (TXCnt%100 == 0 and TXCnt != 0) report();
}

void report() {
    Serial.print(F(">>TX:"             )); Serial.print(TXCnt);
    Serial.print(F("  TXFailed:"  )); Serial.print((float)TXFailedCnt*100/TXCnt); Serial.print('%');
    Serial.print(F("  RX:"             )); Serial.print(RXCnt);
    Serial.print(F("  RXFailed:"  )); Serial.print((float)RXFailedCnt*100/RXCnt); Serial.print('%');
    Serial.print(F("  TOF:"            )); Serial.print((float)(millis() - localtime)/(TXCnt + RXCnt)); Serial.print(F("mS"));
    Serial.println();  
}

void cresson_onReceived() {
  cresson.destID = cresson.sender();
  if ( cresson.available() == sizeof(p)) {
    cresson >> p;
    Serial.println(p);
    RXCnt++;
    cresson << ++p;
    TXCnt++;
  }
}

void cresson_wiringError() {
  Serial.println(F("Error: Please check Cresson wiring"));
}

void cresson_sendFailed() {
  TXFailedCnt++;
  Serial.println(F("Send Failed"));
}