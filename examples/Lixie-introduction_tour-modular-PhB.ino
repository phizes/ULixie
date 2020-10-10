// Lixie-DIAMEX-introduction-tour-PhB
// Baed on Lixie-II introduction tour by Connor Nishijima

void title(){
  Serial.println(F("----------------------------------"));
  Serial.println(F(" Lixie DIAMEX Introduction Tour   "));
  Serial.println(F(" by Phons Bloemen                 "));
  Serial.println(F(" Released under the GPLv3 License "));
  Serial.println(F("----------------------------------"));
}
// Use the DHT11 temp and humidity sensor
#define USE_DHT11 1

// Use the DS3231 clock module
#define USE_DS3231 1

// Use the DS3231 clock module
#define USE_BMP180 0

// Use the DS3231 clock module
#define USE_DS18B20 1

#define LIXIE_PIN   D7 // Pin to drive Lixies (13 on Arduino is D7 on Wemos)
#define NUM_LIXIES 6  // How many Lixies you have
#define LIXIEHW DIAMEX
#define SIX_DIGIT_CLOCK 1
#define MAX_MESG    15
#define RANDOMSEED  A0

#include "Lixie_DIAMEX.h"            // https://github.com/connornishijima/Lixie_II

#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <DNSServer.h>           //  |
#include <ESP8266WebServer.h>    //  |
#include <WiFiUdp.h>             //  |
#include <FS.h>                  // <

#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
// #include <NTPClient.h>           // https://github.com/arduino-libraries/NTPClient
#include <ArduinoJson.h>         // https://github.com/bblanchon/ArduinoJson

#if USE_DHT11
#include <DHTesp.h>
#define DHT11_PIN D4 // 2 on Arduino
#endif

#if USE_DS18B20
#include <DS18B20.h>
#define DS18B20_PIN D2 // 2 on arduin
#endif

#if USE_BMP180
#include <Adafruit_BMP085.h>
#define BMP180_PIN D2
#define BMP180_PIN1 D1
#endif

#if USE_DS3231
#include <MD_DS3231.h>
#include <Wire.h>
#include <SPI.h>
//DS3231 RTC(SDA, SCL);
#endif

// Global variables

char szMesg[MAX_MESG+1] = "";

static const char months[][4] PROGMEM = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
static const char dow[][10] PROGMEM = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday" };

static uint32_t opteffecttime[15] = {40, 200, 600,  800, 400, 400, 250, 250,
                                     40, 40,  150, 30, 130, 60, 40};

// INSTANT CROSSFADE CROSSFADE2 DEEPFADE ONEBYONELEFT ONEBYONERIGHT SCROLLLEFT SCROLLRIGHT
// SCROLLFLAPLEFT SCROLLFLAPRIGHT BLINKER SLOTMACHINE PINBALL GASPUMP SLOTIIMACHINE

// initialization of objects and libraries

Lixie_DIAMEX lix(LIXIE_PIN, NUM_LIXIES);

uint16_t function_wait = 800;
static uint32_t lastTime = 0; // millis() memory
uint16_t treffect = INSTANT;
uint16_t trtype = INSTANT;
uint16_t done = 0;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(RANDOMSEED));
  lix.setDiamex(NUM_LIXIES);
  lix.begin(); // Initialize LEDs
  lix.white_balance(Tungsten100W); // Default
    // Can be: Tungsten40W, Tungsten100W,  
    //         Halogen,     CarbonArc,
    //         HighNoonSun, DirectSunlight,
    //         OvercastSky, ClearBlueSky  
    // 2,600K - 20,000K
  lix.nixie();
  lix.transition_effect(trtype);
  lix.transition_type(trtype);
  lix.transition_time(700);
  title();
	delay(100);
  setuprtc();
  lastTime = millis();
}

void setuprtc () {
 
#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif
#if USE_DS_3231
  RTC.control(DS3231_CLOCK_HALT, DS3231_OFF);
  RTC.control(DS3231_12H, DS3231_OFF);
  if (RTC.status(DS3231_HALTED_FLAG)) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    // RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // RTC.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
#endif
}
 
void loop () {
  static uint8_t  display = 0;  // current display mode

  static bool flasher = false;  // seconds passing flasher
  
  static const char  testdata1_string[][7] = 
    { "678901", "123456", "   312", "1 3 67", "567 10", "      ", "384756"};  
  static const int32_t testdata2_int[7] = 
    { 10, 7776, 0, -8, -21147, 512763, -654 }; 
  static const int32_t testdata3_int[7] = 
    { -1, 1048576, -131072, 262144, 536870912, -2147483548, 65536 };  
  static const float testdata4_float[7] = 
    { 21, 1234567.89, 65432.789, 456.1, 0.0001, 8, 5.005 };
  static const float testdata5_float[7] = 
    { -2.35694, -2.2, -167, -65.6785544, -0.3300, -67.891, 0 };
  static const float testdata6_float[7] =
    { 4.65789e08, 0.00000034571, -0.0007675786, 3.872e06, 6.8806e15, -46576337.7876, -1.876549e34 };
  static const float testdata7_float[7] = 
    { 1e100, 1.75564e-09, -5.332e-09, -1e-10, 3.3333e33, 0.996e-09, -4.123e-07 };
  
  uint32_t t_now, tt;
  float tf;
  uint16_t r,g,b,i,e;
  char numberString[NUM_LIXIES];
  char text[NUM_LIXIES+2];
  text[NUM_LIXIES+1]= '\0';
  
  t_now=millis();

// We are a clock, so adjust the time string if we have to
  if ((t_now - lastTime) >= 1000)
  {
    lix.nixie();
    show_time();
    flasher = !flasher;
    lastTime = t_now;
  }
  if (t_now % 20 == 0) { // 50 FPS
    lix.run();
  }

  if ((t_now % 15000 == 18000) && (done == 0)) { // the 5th second of every 30 seconds
    done = 1;
//    showSerialEverything(t_now); 
    lix.transition_time(opteffecttime[treffect]);
    lix.transition_effect(treffect);
    lix.transition_type(INSTANT);
    for (uint8_t i=0; i < 7; i++) {
        while ((r+g+b) < 700) {
           r = 200 + random(55);
           g = 200 + random(55);
           b = 200 + random(55);
         }
         lix.clear_all();
//         showSerialEverything(t_now);
         lix.color_all(OFF,CRGB(3,0,5));
         lix.color_all(ON,CHSV(r,g,255));
//        lix.write(testdata1_string[i]);
         lix.write(testdata3_int[i]);     
//         lix.write_floatplus(testdata7_float[i],CRGB(r,g,b));
//         lix.write_floatplus(testdata2_float[i],CRGB(r,g,b)); 
         delay(3000);
    }
//    showSerialEverything(t_now);
    lix.nixie();
    lix.transition_effect(trtype);
    lix.transition_type(trtype);
    lix.transition_time(opteffecttime[trtype]);
  }

 if (t_now % 15000 == 4000) {
    sprintf(text,"%2d %3d",treffect,opteffecttime[treffect]/10);
    lix.clear_all();
    delay(400);
    lix.transition_time(opteffecttime[treffect]);
    lix.transition_effect(treffect);
    lix.transition_type(trtype);
//    showSerialEverything(t_now);
    lix.write(text);
    delay(1000);
    lix.clear_all();
    delay(400);
//    showSerialEverything(t_now);
    lix.transition_effect(trtype);
    lix.transition_type(trtype);
    lix.transition_time(opteffecttime[trtype]);
 }  

 if (t_now % 15000 == 14000) { // the 27th second of every 30 seconds
    tt = random(100000);
    for (i = 0; i < NUM_LIXIES; i++) {
       numberString[i] = char ((tt % 10) + 48);
       tt = (uint32_t) (tt / 10);
    }
    r = random(255);
    g = random(255);
    b = random(255);
    lix.color_all(OFF,CRGB(5,3,0));
    lix.color_all(ON,CHSV(r,255,255)); 
    lix.transition_time(opteffecttime[treffect]);
    lix.transition_effect(treffect);
    lix.transition_type(trtype);
//    showSerialEverything(t_now);
    lix.write(numberString);
    delay(1000);
    lix.transition_effect(trtype);
    lix.transition_type(trtype);
    lix.transition_time(opteffecttime[trtype]);
//    showSerialEverything(t_now);
    done = 0;
    treffect = treffect + 1;
    trtype = trtype + 1;
    if (trtype > DEEPFADE) trtype = INSTANT;
    if (treffect > SLOTIIMACHINE) treffect = INSTANT;
 }  
 if (t_now % 15000 == 10000) { // 40th second
      lix.brightness(0.4);
 }
 if (t_now % 15000 == 12000) { // 40th second
      lix.brightness(1.0);
 }
}

char readDigitToChar(uint8_t digit) {
    uint8_t num;
    num = lix.read_digit(digit);
    if (num < 10) {
      return (num + 48);
    } else if (num == 128) {
      return ' ';
    } else {
      return '.';
    }
}

String digitRGBToString(uint8_t digit) {
    char out[30];
    sprintf(out, "[%c-%02x%02x%02x-%02x%02x%02x]",
        readDigitToChar(digit),
       lix.get_color_display(digit,ON).r,
       lix.get_color_display(digit,ON).g,
       lix.get_color_display(digit,ON).b,
       lix.get_color_display(digit,OFF).r,
       lix.get_color_display(digit,OFF).g,
       lix.get_color_display(digit,OFF).b);
    return out;
}

void showSerialEverything(uint32_t t_now) {
    Serial.print(t_now);
    Serial.print(" [");
    Serial.print(lix.readString());
    Serial.print("] ");
    for (uint8_t i=0; i<NUM_LIXIES; i++) {
        Serial.print(digitRGBToString(i));
    }
    Serial.println();
}

// Functions to get Time and Date for the clock

void getTime(char *psz)
// Code for reading clock time
{
  uint32_t  h,m,s;
#if  USE_DS3231
  RTC.readTime();
  h=RTC.h;
  m=RTC.m;
  s=RTC.s;
#else
  s = millis() / 1000;
  m = s / 60;
  h = m / 60;
  m = m % 60;
  s = s % 60;
#endif

// 12 hour format conversion
//  if (clock_config.hour_12_mode == true) {
//    if (hh > 12) {
//      hh -= 12;
//    }
//    if (hh == 0) {
//      hh = 12;
//    }
  sprintf(psz, "%02d%02d%02d", h,m,s);
//  if (!SIX_DIGIT_CLOCK) {
//    tsprinf(psz, "02d%02d", h,m);
//  }  
}

void getDate(char *psz)
// Code for reading clock time
{
  uint32_t s,yy,mm,dd;
#if  USE_DS3231
  dd=RTC.dd;
  mm=RTC.mm;
  yy=RTC.yyyy;
#else
// return unbelievable date
  s = millis();
  dd = (s / 86400) + 30;
  mm = 2;
  yy = 99;
#endif
  sprintf(psz, "%02d%02d%02d", yy,mm,dd);
}

// Functions to show different values for the tour

void show_date() {
  getDate(szMesg);
  lix.write(szMesg); // Update numerals
  Serial.print("DATE : ");
  Serial.println(szMesg);
}

void show_time() {
  getTime(szMesg);
//  if (hh < 6 || hh >= 21) { // Dim overnight from 9PM to 6AM (21:00 to 06:00)
//    if (night_dimming_state == HIGH) { // But only if dimming is enabled
//      lix.brightness(0.4);
//    }
//    else { // If not enabled, full brightness
//      lix.brightness(1.0);
//    }
//  }
//  else { // Or if not in the overnight time window, full brightness
//   lix.brightness(1.0);
//  }
  lix.write(szMesg); // Update numerals
  
//  if (!time_found) { // Cues initial fade in
//    time_found = true;
//    lix.fade_in();
//  }
}
