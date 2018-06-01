#include <crstream.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "myframe.h"

// Cresson instance
#if defined(HAVE_HWSERIAL1)
  crstream<>  cresson(Serial1);
#else
  #include <SoftwareSerial.h>
  #define   CRESSON_RX      7
  #define   CRESSON_TX      8
  SoftwareSerial            Serial1(CRESSON_RX, CRESSON_TX);
  crstream<SoftwareSerial>  cresson(Serial1);
#endif

#define SECOND  1000    // millisecond
#define MINUTE    60    // seconds
#define HOUR    3600    // seconds
#define DAY    86400    // seconds
#define WDTO_SLEEP_FOREVER    (0xFFu)

const uint32_t  sampleInterval = 15*MINUTE;
const PROGMEM char P_date[]   = __DATE__  ;
const PROGMEM char P_time[]   = __TIME__  ;

uint32_t    sample_no = 0;
myframe     payload;

void setup() {
  // cresson setup
  cresson.selfID    = randomID(P_date, P_time) ; // default: 0x0000
  cresson.destID    =         0 ; // default: 0xFFFF
  cresson.autosleep =   true    ;
  cresson.begin();
}

void loop() {

  payload.uptime    = sample_no*sampleInterval + millis()/1000; // off-time + on-time
  payload.value     = sensorRead();
  cresson << payload;

  cresson.update();
  hwInternalSleep(sampleInterval*SECOND);
  sample_no++;
}

// read AVCC
float sensorRead() {
    return (float) hwCPUVoltage()/1000; // volt
}

void hwPowerDown(const uint8_t wdto)
{
  // disable ADC for power saving
  ADCSRA &= ~(1 << ADEN);
  // save WDT settings
  uint8_t WDTsave = WDTCSR;
  if (wdto != WDTO_SLEEP_FOREVER) {
    wdt_enable(wdto);
    // enable WDT interrupt before system reset
    WDTCSR |= (1 << WDCE) | (1 << WDIE);
  } else {
    // if sleeping forever, disable WDT
    wdt_disable();
  }
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
  // restore saved WDT settings
  WDTCSR = WDTsave;
  sei();
  // enable ADC
  ADCSRA |= (1 << ADEN);
}

void hwInternalSleep(unsigned long ms)
{
  // Sleeping with watchdog only supports multiples of 16ms.
  // Round up to next multiple of 16ms, to assure we sleep at least the
  // requested amount of time. Sleep of 0ms will not sleep at all!
  ms += 15u;

  while (ms >= 16) {
    for (uint8_t period = 9u; ; --period) {
      const uint16_t comparatorMS = 1 << (period + 4);
      if ( ms >= comparatorMS) {
        hwPowerDown(period); // 8192ms => 9, 16ms => 0
        ms -= comparatorMS;
        break;
      }
    }
  }
}

uint16_t hwCPUVoltage()
{
  // Measure Vcc against 1.1V Vref
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = (_BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1));
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = (_BV(MUX5) | _BV(MUX0));
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = (_BV(MUX3) | _BV(MUX2));
#else
  ADMUX = (_BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1));
#endif
  // Vref settle
  delay(70);
  // Do conversion
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC)) {};
  // return Vcc in mV
  return (1125300UL) / ADC;
}

