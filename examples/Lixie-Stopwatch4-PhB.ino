/*
 * ULixie-Stopwatch3-Phb
 * Example code to create a menu for Lixies with
 * a rotary encoder-with-button
 * This one uses a polling based approach to read out buttons
 */
#include "MD_UISwitch.h"   //  https://github.com/MajicDesigns/MD_UISwitch
#include "MD_REncoder.h"   //  https://github.com/MajicDesigns/MD_Rencoder 
#include "Lixie_DIAMEX.h"  //  https://github.com/phizes/Ulixie

#define CLICKS_PER_STEP   4   // this number depends on your rotary encoder 

#define LIXIE_PIN   D7    // Pin to drive Lixies (13 on Arduino is D7 on Wemos)
#define NUM_LIXIES 6      // How many Lixies you have
#define LIXIEHW DIAMEX    // Type of Lixies
#define SIX_DIGIT_CLOCK 1 //
#define MAX_MESG    15    //

#define RANDOMSEED  A0    // 

// Global variables

const uint8_t ROTARY_PIN1 = D3;
const uint8_t ROTARY_PIN2 = D4;
MD_REncoder  rotate(ROTARY_PIN1, ROTARY_PIN2);

const uint8_t BUTTON1_PIN = D5;
const uint8_t BUTTON2_PIN = D6;
MD_UISwitch_Digital buttonA(BUTTON1_PIN, LOW);
MD_UISwitch_Digital buttonB(BUTTON2_PIN, LOW);

ULixie lix(LIXIE_PIN, NUM_LIXIES);

char szMesg[MAX_MESG+1] = "";
static uint32_t lastTime = 0;
uint16_t treffect = INSTANT;
uint16_t trtype = INSTANT;
uint16_t done = 0;
uint8_t fsm_state = 0;
uint32_t t_start = 0;
uint32_t t_lap = 0;
uint32_t t_stop = 0;
uint32_t t_lastAction = 0;
uint32_t t_countdown = 240000;

// Initialization

void setup() {
  Serial.begin(115200);
  delay(50);
  
  randomSeed(analogRead(RANDOMSEED));
  lix.setDiamex(NUM_LIXIES);
  lix.begin(); // Initialize LEDs
  lix.white_balance(Tungsten100W); 
  lix.nixie();
  lix.transition_effect(1);
  lix.transition_type(1);
  lix.transition_time(700);

//  setuprtc();
  lastTime = millis();

  rotate.begin();
  buttonA.begin();
  buttonB.begin();

  buttonA.setPressTime(200);        // 150
  buttonA.setLongPressTime(700);   // 600
  buttonA.setDoublePressTime(300);
  buttonA.enableRepeat(false);  
  buttonA.enableDoublePress(false);

  buttonB.setPressTime(300);
//  buttonB.setRepeatTime(700);
  buttonB.setLongPressTime(900);
  buttonB.setDoublePressTime(600);
//  buttonB.enableRepeatResult(false);
  buttonB.enableRepeat(false);
  buttonB.enableDoublePress(false);

//  S.begin();
//  S.enableDoublePress(false);
//  S.enableLongPress(false);
//  S.enableRepeat(false);
//  S.enableRepeatResult(true);

  title();
}

void getTime(char *psz) {
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
  sprintf(psz, "%02d%02d%02d", h,m,s);
//  if (!SIX_DIGIT_CLOCK) {
//    tsprinf(psz, "02d%02d", h,m);
//  }  
}

void show_time() {
  getTime(szMesg);
  lix.write(szMesg);
}

void convert_time(char *psz, uint32_t tt) {
  uint32_t hh, s, m, h;
  hh = tt / 10;
  s = hh / 100;
  m = s / 60;
  h = m / 60;
  hh = hh % 100;
  s = s % 60;
  m = m % 60;
  h = h % 24;
  if (h > 0) {
    sprintf(psz, "%1d%02d%02d%01d", h, m, s, hh / 10);
  } else {
    sprintf(psz, "%2d%02d%02d", m,s,hh);
  }
}

// the loop funtion

void loop() {
  uint32_t t_now;
  
  t_now=millis();
  
  if (t_now % 10 == 0) { // 100 FPS
    lix.run();
  }
  
  switch (fsm_state) {
    case 0:
// clock mode 
      if ((t_now - lastTime) >= 1000) {
        show_time();
        lastTime = t_now;
      }
      break;
   case 10:
// stopwatch mode, not running
      if (t_now - t_lastAction > 60000) {
         t_lastAction  = t_now;
         lix.nixie();
         fsm_state = 0;               
      }
      break;
   case 11:
// stopwatch mode, running
      if ((t_now - lastTime) >= 10) {
         convert_time(szMesg,t_now - t_start);
         lix.write(szMesg);
         lastTime = t_now;
      }
      t_lastAction = t_now;
      break;
   case 12:
// stopwatch mode, lap time displaying
      t_lastAction = t_now;
      break;
   case 13:
// stopwatch mode, stop time displaying
      if (t_now - t_lastAction > 60000) {
         Serial.println("Inactivity timer expired");
         t_lastAction = t_now;
         lix.nixie();
         fsm_state = 0;
      }
      break;
   case 14:
      break;
   case 20:
// countdown mode, not running
      if (t_now - t_lastAction > 60000) {
         Serial.println("Inactivity timer expired");
         t_lastAction  = t_now;
         lix.nixie();
         fsm_state = 0;               
      }
      break;
   case 21:
// countdown mode, running
      if ((t_now - lastTime) >= 10) {
         convert_time(szMesg,t_stop - t_now);
         lix.write(szMesg);
         lastTime = t_now;
      }
      if ((t_stop - t_now) < 1) {
         Serial.println("Countdown expired");
         lix.transition_time(600);
         lix.transition_type(BLINKER);
         lix.write("000000");
         lix.run();
         delay(1000);
         lix.transition_time(50);
         lix.transition_type(INSTANT);
         convert_time(szMesg, t_countdown);
         lix.write(szMesg);
         fsm_state = 20;
      }
      t_lastAction = t_now;
      break;
   case 23:
// countdown mode, stopped, remaining time displaying
      if (t_now - t_lastAction > 60000) {
         Serial.println("Inactivity timer expired");
         t_lastAction = t_now;
         lix.nixie();
         fsm_state = 0;
      }
      break;
  }

  uint8_t x = rotate.read();
  MD_UISwitch::keyResult_t kA = buttonA.read();
  MD_UISwitch::keyResult_t kB = buttonB.read();
  
  if (x) {
    Serial.print(x == DIR_CW ? "+1" : "-1");
    Serial.print(" ");
    Serial.print(rotate.speed());
    Serial.print(" ");
  }
  switch(kA) {
    case MD_UISwitch::KEY_DOWN:     
       Serial.print("A KEY_DOWN ");
       Serial.println(fsm_state);
       break;    
    case MD_UISwitch::KEY_DPRESS:     
       Serial.print("A KEY_DPRESS ");
       Serial.println(fsm_state);
       break;
    case MD_UISwitch::KEY_PRESS:     
       Serial.print("A KEY_PRESS ");
       Serial.println(fsm_state);
       switch (fsm_state) {
          case 11:
// stopwatch, lap time pressed
             t_lap = t_now;
             convert_time(szMesg,t_lap - t_start);
             lix.color_all(ON,CRGB(255,255,128));
             lix.write(szMesg);
             lastTime = t_now;
             fsm_state = 12;           
             break;
          case 12:
// stopwatch, lap time cleared               
             fsm_state = 11;
             break;
          case 13:
// stopwatch reset
             t_start = t_now;
             lix.color_all(ON,CRGB(255,255,0));
             lix.write("000000");
             fsm_state = 10;
             break;
          case 14:
// stopwatch  lap time presed, stop time memorized
             convert_time(szMesg,t_stop - t_start);
             lix.color_all(ON,CRGB(255,255,0));
             lix.write(szMesg);
             lastTime = t_now;
             fsm_state = 13;
             break;
          case 21:
             convert_time(szMesg,t_countdown);
             lix.color_all(ON,CRGB(255,0,255));
             lix.write(szMesg);
             lastTime = t_now;
             fsm_state = 20;
             break;
          default:
             break;
       }
       break;
    case MD_UISwitch::KEY_LONGPRESS: 
       Serial.print("A KEY_LONG ");
       Serial.println(fsm_state);
       switch (fsm_state) {
           case 0:
// mode change to stopwatch
             lix.color_all(ON,CRGB(255,255,0));
             lix.write("000000");
             lastTime = t_now;
             fsm_state = 10;
             break;
           case 10:
// mode change to countdown
             convert_time(szMesg, t_countdown);
             lix.color_all(ON,CRGB(255,0,255));
             lix.write(szMesg);
             lastTime = t_now;
             fsm_state = 20;
             break;
           case 20:
// mode change to clock
             t_lap = t_now;
             lix.nixie();
             fsm_state = 0;
             break;
           default:
             break;
       }
    default:
       break;
  }
  switch(kB) { 
     case MD_UISwitch::KEY_DOWN:     
       Serial.print("B KEY_DOWN ");
       Serial.println(fsm_state);
       break;   
    case MD_UISwitch::KEY_DPRESS:     
       Serial.print("B KEY_DPRESS ");
       Serial.println(fsm_state);
       break;   
    case MD_UISwitch::KEY_LONGPRESS:     
       Serial.print("B KEY_LONGPRESS ");
       Serial.println(fsm_state);
       break;
    case MD_UISwitch::KEY_PRESS:
       Serial.print("B KEY_PRESS ");
       Serial.println(fsm_state);
       switch (fsm_state) {
          case 10:
// stopwatch, start pressed from reset
             t_start = t_now;
             fsm_state = 11;
             break;
          case 11:
// stopwatch, stop pressed
             t_stop = t_now;
             convert_time(szMesg,t_stop - t_start);
             lix.color_all(ON,CRGB(255,255,0));
             lix.write(szMesg);
             fsm_state = 13;
             break;
          case 12:
             t_stop = t_now;
             fsm_state = 14;
             break;
          case 13:
             t_start = t_now - (t_stop - t_start);
             fsm_state = 11;
             break;
          case 14:
             convert_time(szMesg,t_stop - t_start);
             lix.color_all(ON,(128,255,0));
             lix.write(szMesg);
             t_start = t_now - (t_stop - t_start);
             fsm_state = 12;
             break;
          case 20:
             t_start = t_countdown;
             t_stop = t_now + t_start;
             fsm_state = 21;
             break;
          case 21:
             t_start = t_stop - t_now;
             fsm_state = 23;
             break;
          case 23:
             t_stop = t_now + t_start;
             fsm_state = 21;
             break;
          default:
             break;
       }
    default:
       break;
  } 
}

void title(){
  Serial.println(F("----------------------------------"));
  Serial.println(F(" ULixie Stopwatch v3              "));
  Serial.println(F(" by Phons Bloemen                 "));
  Serial.println(F(" Released under the GPLv3 License "));
  Serial.println(F("----------------------------------"));
}
