// Ulixie-introduction-tour-PhB
// Based on Lixie-II introduction tour by Connor Nishijima
// This is a somehow 'test-work-test-inprogress'

void title(){
  Serial.println(F("----------------------------------"));
  Serial.println(F(" ULixie Introduction Tour         "));
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

#define DATA_PIN   D7 // Pin to drive Lixies (13 on Arduino is D7 on Wemos)
#define NUM_LIXIES 6  // How many Lixies you have
#define LIXIEHW DIAMEX
#define SIX_DIGIT_CLOCK 1
#define MAX_MESG    15
#define RANDOMSEED  A0

#include <ULixie.h>            // https://github.com/phizes/ULixie

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

// global variables

char szMesg[MAX_MESG+1] = "";

static const char months[][4] PROGMEM = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
static const char dow[][10] PROGMEM = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday" };

static uint32_t opteffecttime[12] = {40, 700, 250, 250, 50, 40,
                                     40,  30, 130, 60, 40, 150};

// INSTANT CROSSFADE SCROLLLEFT SCROLLRIGHT FLAPPER SCROLLFLAPLEFT
// SCROLLFLAPRIGHT SLOTMACHINE PINBALL GASPUMP SLOTIIMACHINE BLINKER

// initialization of objects and libraries

ULixie lix(DATA_PIN, NUM_LIXIES);

#ifdef USE_DHT11
DHTesp dht;
#endif

uint16_t function_wait = 800;
static uint32_t lastTime = 0; // millis() memory
uint16_t effect = PINBALL;
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
  lix.transition_type(CROSSFADE);
  lix.transition_time(700);
  title();
	delay(100);
  setuprtc();
#if USE_DHT11
 dht.setup(17, DHTesp::DHT22); // Connect DHT sensor to GPIO 17
 delay(dht.getMinimumSamplingPeriod());
#endif
// run_demo();

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
  static const float testdata1_float[7] = {21, 1234567.89, 65432.789, 456.1, 0.0001, 8, 5.005};
  static const float testdata2_float[7] = {2.35694, 2.2, 167, 0.45, 0.3300, 67.890, 0};
  static const char  testdata[][7] = {
    "678901", "123456", "   312", "1 3 67", "567 10", "      ", "384756"};
  uint32_t t_now, tt;
  float tf;
  uint16_t r,g,b,i,e;
  char numberString[NUM_LIXIES];
  
  t_now=millis();

// We are a clock, so adjust the time string if we have to
  if ((t_now - lastTime) >= 1000)
  {
    lix.transition_type(CROSSFADE);
    lix.nixie();
    show_time();
    flasher = !flasher;
    lastTime = t_now;
  }
  if (t_now % 20 == 0) { // 50 FPS
    lix.run();
  }
//  if (t_now - settingls_last_update > 5000 && settings_changed == true) {
//    settings_changed = false;
//    save_settings();
//  }
  
  if ((t_now % 45000 == 5000) && (done == 0)) { // the 8th second of every 40 seconds
    done = 1;
    showSerialEverything(t_now); 
    lix.transition_type(INSTANT);
    lix.transition_time(opteffecttime[effect]);
    lix.transition_effect(effect);
    lix.clear_all();
    for (uint8_t i=0; i < 7; i++) {
         showSerialEverything(t_now);
         lix.color_all(OFF,CRGB(10,0,16));
         lix.color_all(ON,CRGB(r,g,b));
         lix.write(testdata[i]);
//        lix.write_float(testdata1_float[i],2);
//         lix.write_float(testdata2_float[i],2);     
//         lix.write_floatplus(testdata1_float[i],CRGB(r,g,b));
         delay(1000);
         lix.write_floatplus(testdata2_float[i],CRGB(r,g,b)); 
         r = random(255);
         g = random(255);
         b = random(255);
         delay(1000);
    }
    showSerialEverything(t_now);
    lix.nixie();
    lix.transition_effect(INSTANT);
    lix.transition_type(CROSSFADE);
    lix.transition_time(13000);
    
    //    effect = effect + 1;
//    if (effect > BLINKER) {
//      effect = INSTANT;
//    }
  }
  
 if (t_now % 45000 == 37000) { // the 8th second of every 40 seconds
    tt = random(100000);
    for (i = 0; i < NUM_LIXIES; i++) {
       numberString[i] = char ((tt % 10) + 48);
       tt = uint32_t (tt / 10);
    }
    r = random(255);
    g = random(255);
    b = random(255);
    lix.color_all(OFF,CRGB(15,10,0));
    lix.color_all(ON,CHSV(r,255,255)); 
    lix.transition_time(opteffecttime[effect]);
    lix.transition_effect(effect);
    showSerialEverything(t_now);
    lix.write(numberString);
    delay(1000);
    showSerialEverything(t_now);
    lix.transition_effect(INSTANT);
    lix.transition_type(CROSSFADE);
    lix.transition_time(6000);
    lix.nixie();
    done = 0;
    effect = effect + 1;
    if (effect > BLINKER) {
      effect = INSTANT;
    }
    if (effect == INSTANT) effect = 3;
    if (effect == FLAPPER) effect++;
  }

  
 if (t_now % 45000 == 34000) { // 40th second
      lix.brightness(0.4);
 }
 if (t_now % 45000 == 4000) { // 40th second
      lix.brightness(1.0);
 }
  
if (t_now % 60000 == 74000) { // 10th second  
//    color_for_mode();
    lix.transition_type(INSTANT);
    lix.color_all_dual(ON,CRGB(0,0,0),CRGB(255,250,0));
    show_temp();    
    delay(2100);
  }
  
  if (t_now % 60000 == 64000) { // 50th second
    tt = random(100000) / 10000;
    r = random(255);
    g = random(255);
    b = random(255);
//    color_for_mode();
    lix.transition_type(INSTANT);
    lix.color_all(OFF,CRGB(0,0,0));
    lix.color_all(ON,CRGB(r,g,b));  
    for (i = 0; i < 5; i++) {
       lix.write_float(tt,2);
       tt *= random(30);
       delay(500);
    }
  }

  if (t_now % 60000 == 80000) { // 20th second
//    color_for_mode();
    lix.transition_type(INSTANT);
    lix.color_all_dual(ON,CRGB(0,0,0),CRGB(25,0,200));
    show_rh();
    delay(2100);
  }

  if (t_now % 60000 == 90000) { // 30th second
//    color_for_mode();
    lix.transition_type(INSTANT);
    lix.color_all_dual(ON,CRGB(10,10,10),CRGB(100,100,100));
    show_wsp();
    delay(2100);
  }

  if (t_now % 60000 == 100000) { // 40th second
//    color_for_mode();
    lix.transition_type(INSTANT);
    lix.color_all_dual(ON,CRGB(0,0,0),CRGB(0,200,0));
    show_aex();
    delay(3500);
  }

    if (t_now % 60000 == 110000) { // 50th second
//    color_for_mode();
    lix.transition_type(INSTANT);
    lix.color_all(OFF,CRGB(0,0,0));
    lix.gradient_rgb(ON, CRGB(200,0,0), CRGB(0,200,0));
    show_date();
    delay(2100);
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

void show_temp() {
  float tt = 0;
#if USE_DHT11
  tt = dht.getTemperature();
#elif USE_DS3231
  tt=25.377;
#else
  tt=22.011;
#endif
  if (!SIX_DIGIT_CLOCK) {
    lix.write_float(tt, 1);
  } else {
    lix.write_float(tt, 2);
  }
  Serial.print("TEMP : ");
  Serial.println(tt);
}

void show_rh() {
  float tt = 0;
#if USE_DHT11
  tt = dht.getHumidity();
#elif USE_DS3231
  tt=26.800;
#else
  tt=37.034;
#endif
  if (!SIX_DIGIT_CLOCK) {
    lix.write_float(tt, 1);
  } else {
    lix.write_float(tt, 2);
  }
  Serial.print("RH   : ");
  Serial.println(tt);
}

void show_wsp() {
  float tt = 0;
#if USE_DS3231
  tt = 1005.376;
#else
  tt = 997.008;
#endif
  if (!SIX_DIGIT_CLOCK) {
    lix.write_float(tt, 0);
  } else {
    lix.write_float(tt, 1);
  }
  Serial.print("PRESS: ");
  Serial.println(tt);
}

void show_aex()
{
  float tt = 0;
#if USE_DS3231
  tt=575.89;
#else
  tt=540.61;
#endif
  lix.write(szMesg);
//  if (!SIX_DIGIT_CLOCK) {
//    lix.write_float(tt, 1)
//  } else {
  lix.write_floatplus(tt, 2);
//  }
  Serial.print("AEX: ");
  Serial.println(tt);
}

/*
  void show_ping() {
  bool ret = Ping.ping(PING_SITE,PING_COUNT);
  if(ret == true){
    int avg_time_ms = Ping.averageTime();

    uint16_t avg_shift = constrain(avg_time_ms-PING_GOOD,0,65535);
    uint16_t bad_shift = constrain(PING_BAD - PING_GOOD,0,65535);

    float rating = constrain(float(avg_shift) / float(bad_shift), 0, 1);
    
    lix.color(255*rating, 255*(1-rating), 0);
    if (!SIX_DIGIT_CLOCK) {
      lix.write_float(avg_time_ms, 1)
    } else {
      lix.write_float(avg_time_ms, 2);
    }
    Serial.print("PING: ");
    Serial.println(avg_time_ms);
  } else {
    // ERROR
    lix.color(255, 0, 0);
    if (!SIX_DIGIT_CLOCK) {
      lix.write("0000");
    } else {
      lix.write(" 0000 ");
    }
  }
}
*/

void run_demo(){
  lix.color_all(ON, CRGB(0,255,255)); // Start with cyan color
  delay(1000);
  
  Serial.println("This is a tour of the basic and advanced features of the ULixie / Lixie II library!\n");
  delay(2000);
  
  Serial.println("Let's begin!\n");
  delay(3000);

  lix.write_digit(0,7);
  delay(function_wait);
   
  lix.write_digit(1,8);
  delay(function_wait);
  
  lix.write_digit(4,9);
  delay(function_wait);

  lix.write_digit(1,5);
  delay(function_wait);
  
  Serial.println("Lixies can take integers, char arrays, or floats as input with lix.write() and lix.write_float(). (Any non-numeric chars are ignored)\n");
  lix.write_float(2.0, 1);
  delay(function_wait);
  
  Serial.println("For the Nixie fans, Lixies offers the lix.nixie() function to shortcut you to pleasant Nixie tube colors!\n");
  lix.nixie();
  lix.write(123456);
  delay(function_wait);
  
  Serial.println("Lixies operate with 'ON' and 'OFF' colors.");
  Serial.println("Currently, the ON color is orange, and the OFF");
  Serial.println("color is a dark blue.\n");
  delay(function_wait);
  
  Serial.println("They can be configured independently in the lix.color_all(<ON/OFF>, CRGB col) function.");
  Serial.println("I've now used color_all(OFF, CRGB(0,8,0)); to change the blue to a dark green!\n");
  lix.color_all(OFF, CRGB(0,10,0));
  delay(function_wait);
  
  Serial.println("Lixies can also have two colors per digit by using lix.color_all_dual(ON, CRGB col1, CRGB col2);!\n");
  lix.color_all_dual(ON, CRGB(255,0,0), CRGB(0,0,255));
  delay(function_wait);
  
  Serial.println("Individual displays can also be colored with lix.color_display(index, <ON/OFF>, CRGB col)!\n");
  lix.color_display(1, ON, CRGB(255,0,0));
  lix.color_display(1, OFF, CRGB(0,0,10));
  delay(function_wait);
  
  Serial.println("Gradients are possible with lix.gradient_rgb(<ON/OFF>, CRGB col1, CRGB col2)!\n");
  lix.color_all(OFF, CRGB(0,0,0)); // Remove OFF color
  lix.gradient_rgb(ON, CRGB(255,0,255), CRGB(0,255,255));
  delay(function_wait);
  
  Serial.println("Lixies can also show 'streaks' at any position between left (0.0) and");
  Serial.println("right (1.0) with lix.streak(CRGB col, float pos, uint8_t blur);\n");
  lix.stop_animation(); // Necessary to prevent rendering of current numbers
  float iter = 0;
  uint32_t t_start = millis();
  while(millis() < t_start+function_wait){
    float pos = (sin(iter) + 1)/2.0;
    lix.streak(CRGB(0,255,0), pos, 8);
    iter += 0.02;
    delay(1);
  }

  Serial.println("Here's lix.streak() with 'blur' of '1' (none)\n");
  t_start = millis();
  while(millis() < t_start+function_wait){
    float pos = (sin(iter) + 1)/2.0;
    lix.streak(CRGB(0,255,0), pos, 1);
    iter += 0.02;
    delay(1);
  }

  Serial.println("These can be used to show a progress percentage, or 'busy' indicator while WiFi connects.\n");
  delay(3000);
  Serial.println("One shortcut to this functionality is lix.sweep_color(CRGB col, uint16_t speed, uint8_t blur);\n");
  for(uint8_t i = 0; i < 5; i++){
    lix.sweep_color(CRGB(0,0,255), 20, 5);
  }
  lix.start_animation(); // This resumes "number mode" after we stopped it above
  
  lix.nixie();
  Serial.println("Transitions between numbers can also be modified.");
  Serial.println("By default Lixie II uses a 250ms crossfade, but this can");
  Serial.println("be changed with lix.transition_time(ms) or lix.transition_type(<INSTANT/CROSSFADE>);\n");
  
  Serial.println("CROSSFADE...");
  for(uint8_t i = 0; i < 30; i++){
    lix.write(i);
    delay(250);
  }
  Serial.println("INSTANT!\n");
  lix.transition_type(INSTANT);
  for(uint8_t i = 0; i < 30; i++){
    lix.write(i);
    delay(250);
  }

  Serial.println("And to end our guided tour, here's a floating point of seconds elapsing, using lix.rainbow(uint8_t hue, uint8_t separation);");
  delay(3000);
  
  t_start = millis();
  float hue = 0;
  while(millis() < t_start+10000){
    uint32_t millis_passed = millis()-t_start;
    float seconds = millis_passed / 1000.0;
    lix.write_float(seconds, 2);
    lix.rainbow(hue, 20);
    hue += 0.1;
    delay(1);
  }
  lix.write_float(10.0, 2);
  delay(function_wait);
  
  Serial.println("\n\n");
}
