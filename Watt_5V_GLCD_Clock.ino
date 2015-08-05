//#define DEBUG

#include <avr/sleep.h>
#include <avr/power.h>

#include <avr/eeprom.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#include <openGLCD.h>    // openGLCD library   
#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>

#include "DHT.h"

#define DHTPIN 3
#define DHTPOWER 2
#define DHTTYPE DHT22

int16_t TimeZone;
uint16_t TimeZoneAddr = 1;

// for voltage-measurement
#define voltage_pin A7
#define voltage_r1 1003000.0
#define voltage_r2  102500.0
#define resistorFactor (1023.0 * (voltage_r2 / (voltage_r1 + voltage_r2))) / 1.0

DHT dht(DHTPIN, DHTTYPE);

static time_t prevtime;
float h, t, vbat, temp;

#define LED_MAX_COUNT 1
uint8_t led_count = 0;
bool led_on = false;

void DHT_on() {
  digitalWrite(DHTPOWER, HIGH);
  
  pinMode(DHTPIN, INPUT_PULLUP);
  
}

void DHT_off() {
  digitalWrite(DHTPOWER, LOW);

  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
}

void LED_on() {
  digitalWrite(13, HIGH);
  led_on = true;

#ifdef DEBUG
  Serial.println("LED on");
#endif
}

void LED_off() {
  digitalWrite(13, LOW);
  led_on = false;

#ifdef DEBUG
  Serial.println("LED off");
#endif
}

void setup() {
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  
  pinMode(A6, OUTPUT);
  digitalWrite(A6, LOW);
  /*
    // is only run once for first EEPROM-init
    while (!eeprom_is_ready());
    eeprom_write_word((uint16_t*)TimeZoneAddr, 7200);
  */

  Serial.begin(9600);

  setSyncProvider(RTC.get);   // the function to get the time from the RTC

  /*
    if (timeStatus() != timeSet) {
      RTC.set(DateTime(__DATE__, __TIME__));
    }
  */

  GLCD.Init(); // initialize the display

  analogReference(INTERNAL);
  pinMode(voltage_pin, INPUT);
  vbat = analogRead(voltage_pin) / resistorFactor;// * 1.1;

  pinMode(DHTPOWER, OUTPUT);

  DHT_on();

  dht.begin();

  h = dht.readHumidity();
  t = dht.readTemperature();

  DHT_off();

  power_adc_disable(); // we ned Analog-Digital-Converter for Voltage-Check .. but not all the time ..
  power_spi_disable();
  power_timer1_disable();
  power_timer2_disable();
  
  pinMode(voltage_pin, OUTPUT);
  digitalWrite(voltage_pin, LOW);
}

void  loop() {
  time_t current_time;
  char buf[12];
  char dht_str[6];
  uint8_t s;

  if (Serial.available()) {
    processSyncMessage();
  }

  if (prevtime != now()) { // if 1 second has gone by, update display
    prevtime = now();
    s = second();    

    while (!eeprom_is_ready());
    TimeZone = eeprom_read_word((uint16_t*)TimeZoneAddr);

    current_time = prevtime + TimeZone;

#ifdef DEBUG
    Serial.print(F("T1: "));
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour(current_time), minute(), s);
    Serial.println(buf);
#endif

    current_time += IsDst(hour(current_time), day(current_time), month(current_time), weekday(current_time)) ? 60 * 60 : 0;

    GLCD.SelectFont(lcdnums12x16);

    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour(current_time), minute(), s);
#ifdef DEBUG
    Serial.print(F("T2: "));
    Serial.println(buf);
#endif
    GLCD.DrawString(buf, gTextfmt_center, gTextfmt_row(0));

    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year(current_time), month(current_time), day(current_time));
    //    #ifdef DEBUG
    //    Serial.println(buf);
    //    #endif
    GLCD.DrawString(buf, gTextfmt_center, gTextfmt_row(1));

    GLCD.DrawString(F("    "), gTextfmt_center, gTextfmt_row(2));
    GLCD.DrawString(ftoa(dht_str, h, 1), gTextfmt_center, gTextfmt_row(2));

    GLCD.DrawString(F("    "), gTextfmt_center, gTextfmt_row(3));
    GLCD.DrawString(ftoa(dht_str, t, 1), gTextfmt_center, gTextfmt_row(3));

    GLCD.SelectFont(utf8font10x16);
    GLCD.DrawString(F("Hum:"),  gTextfmt_left,   gTextfmt_row(2));
    GLCD.DrawString(F("% Rh"),  gTextfmt_col(8), gTextfmt_row(2));
    GLCD.DrawString(F("Temp:"), gTextfmt_left,   gTextfmt_row(3));
    GLCD.DrawString(F("* C"),   gTextfmt_col(8), gTextfmt_row(3));

    GLCD.SelectFont(Wendy3x5);
    GLCD.DrawString(F("     "), gTextfmt_right, gTextfmt_bottom);
    GLCD.DrawString(ftoa(dht_str, vbat, 3), gTextfmt_right, gTextfmt_bottom);

    // swith LED
    if ((s % 10 == 0) && (led_count == 0)) {
      LED_on();
    }
  }

  led_count += (led_count < LED_MAX_COUNT) && led_on ? 1 : 0;
  
  if (led_count >= LED_MAX_COUNT) {
    LED_off();
    led_count = 0;
  }

  if (s == 57) {
    //  if (s == 7 | s == 17 | s == 27 | s == 37 | s == 47 | s == 57) {
#ifdef DEBUG
    Serial.println("turning on DHT11 & ADC");
#endif

    DHT_on();

    power_adc_enable();
    pinMode(voltage_pin, INPUT);
  }

  if (s == 0) {
    //  if (s % 10 == 0) {
#ifdef DEBUG
    Serial.println("measuring");
#endif

    temp = dht.readHumidity();
    h = isnan(temp) ? h : temp;

    temp = dht.readTemperature();
    t = isnan(temp) ? t : temp;

    analogReference(INTERNAL);
    vbat = analogRead(voltage_pin) / resistorFactor * 1.1;

#ifdef DEBUG
    Serial.print("s   : "); Serial.println(s);

    Serial.print("h   : "); Serial.println(h);
    Serial.print("t   : "); Serial.println(t);
    Serial.print("vbat: "); Serial.println(vbat);
#endif

#ifdef DEBUG
    Serial.println("turning off DHT & ADC");
#endif

    DHT_off();

    power_adc_disable();
    pinMode(voltage_pin, OUTPUT);
    digitalWrite(voltage_pin, LOW);
  } else {
    delay(250);
  }
}

