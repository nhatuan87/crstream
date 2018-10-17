#include <crstream.h>
#include <uniqueID.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

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
  cresson.mhmode    = MHREPEATER;
  cresson.begin();
  while (!cresson.isAlive()) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  cpuPowerDown();
}

void loop() {

}

void cpuPowerDown() {
  // disable ADC for power saving
  ADCSRA &= ~(1 << ADEN);
  // if sleeping forever, disable WDT
  wdt_disable();
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_enable();
#if defined __AVR_ATmega328P__
  sleep_bod_disable();
#endif
  // Enable interrupts & sleep until WDT or ext. interrupt
  sei();
  // Directly sleep CPU, to prevent race conditions!
  // Ref: chapter 7.7 of ATMega328P datasheet
  sleep_cpu();
  sleep_disable();
  // restore previous WDT settings
  cli();
  wdt_reset();
  // enable WDT changes
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  sei();
  // enable ADC
  ADCSRA |= (1 << ADEN);
}
