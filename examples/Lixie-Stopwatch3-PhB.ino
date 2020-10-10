/*
 * ULixie-Rotary-Button-Menu-Phb
 * Example code to create a menu for Lixies with
 * a rotary encoder-with-button
 * This one uses an interrupt drive approach to read out buttons
 * 
 */
#include "Button2.h"         //  https://github.com/LennartHennigs/Button2
#include "ESPRotary.h"       //  https://github.com/LennartHennigs/ESPRotary 
#include "ULixie.h"          //  https://github.com/phizes/Ulixie

#define CLICKS_PER_STEP   4   // this number depends on your rotary encoder 

#define LIXIE_PIN   D7    // Pin to drive Lixies (13 on Arduino is D7 on Wemos)
#define NUM_LIXIES 6      // How many Lixies you have
#define LIXIEHW DIAMEX    // Type of Lixies
#define SIX_DIGIT_CLOCK 1 //
#define MAX_MESG    15    //

#define RANDOMSEED  A0    // 

#define ROTARY_PIN1 D3
#define ROTARY_PIN2 D4
#define BUTTON1_PIN D5        // the button on the rotary encoder
#define CLICKS_PER_STEP   4   // this number depends on your rotary encoder 

#define BUTTON2_PIN D6     // another button

// Global variables

Button2 buttonA  = Button2(BUTTON1_PIN);
Button2 buttonB  = Button2(BUTTON2_PIN);
ESPRotary rotate = ESPRotary(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP);

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
  
  buttonA.setChangedHandler(hChanged);
  buttonA.setPressedHandler(hPressed);
  buttonA.setReleasedHandler(hReleased);
  buttonA.setTapHandler(hTap);
  buttonA.setClickHandler(hClick);
  buttonA.setLongClickHandler(hLongClick);
  buttonA.setDoubleClickHandler(hDoubleClick);
  buttonA.setTripleClickHandler(hTripleClick);
 
  buttonB.setChangedHandler(hChanged);
  buttonB.setPressedHandler(hPressed);
  buttonB.setReleasedHandler(hReleased);
  buttonB.setTapHandler(hTap);
  buttonB.setClickHandler(hClick);
  buttonB.setLongClickHandler(hLongClick);
  buttonB.setDoubleClickHandler(hDoubleClick);
  buttonB.setTripleClickHandler(hTripleClick);

  rotate.setChangedHandler(hRotate);
  rotate.setLeftRotationHandler(hShowDirection);
  rotate.setRightRotationHandler(hShowDirection);

  while (!Serial) {
  }
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
  
  buttonA.loop();
  buttonB.loop();
  rotate.loop();
  
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
}

// Callback functions for buttons

void hPressed(Button2& btn) {
    uint32_t t_now = millis();
    Serial.print(fsm_state);
    if (btn == buttonA) {
      Serial.print(" A");
    } else if (btn == buttonB) {
      Serial.print(" B");
    }
    Serial.println(" pressed");
    if (btn == buttonA) {
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
    } else if (btn == buttonB) { 
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
    }
}
void hReleased(Button2& btn) {
    Serial.print(fsm_state);
    if (btn == buttonA) {
      Serial.print(" A");
    } else if (btn == buttonB) {
      Serial.print(" B");
    }
    Serial.print(" released: ");
    Serial.println(btn.wasPressedFor());
}
void hChanged(Button2& btn) {
    Serial.print(fsm_state);
    if (btn == buttonA) {
      Serial.print(" A");
    } else if (btn == buttonB) {
      Serial.print(" B");
    }
    Serial.println(" changed");
}
void hClick(Button2& btn) {
    Serial.print(fsm_state);
    if (btn == buttonA) {
      Serial.print(" A");
    } else if (btn == buttonB) {
      Serial.print(" B");
    }
    Serial.println(" click");
}
void hLongClick(Button2& btn) {
    uint32_t t_now = millis();
    Serial.print(fsm_state);
    if (btn == buttonA) {
      Serial.print(" A");
    } else if (btn == buttonB) {
      Serial.print(" B");
    }
    Serial.println(" long click\n");
    if (btn == buttonA) {
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
    }
}
void hDoubleClick(Button2& btn) {        
    Serial.print(fsm_state);
    if (btn == buttonA) {
      Serial.print(" A");
    } else if (btn == buttonB) {
      Serial.print(" B");
    }
    Serial.println(" double click");
}
void hTripleClick(Button2& btn) {
    Serial.print(fsm_state);
    if (btn == buttonA) {
      Serial.print(" A");
    } else if (btn == buttonB) {
      Serial.print(" B");
    }
    Serial.println(" triple click");
}
void hTap(Button2& btn) {
    Serial.print(fsm_state);
    if (btn == buttonA) {
      Serial.print(" A");
    } else if (btn == buttonB) {
      Serial.print(" B");
    }
    Serial.println(" tap");
}
void hRotate(ESPRotary& r) {
    Serial.print(fsm_state);
    Serial.print(" ROTARY TURNED");
    Serial.println(r.getPosition());
}
void hShowDirection(ESPRotary& r) {
    Serial.print(fsm_state);
    Serial.print(" ROTARY TURNED");
    Serial.println(r.directionToString(r.getDirection()));
}
void title(){
  Serial.println(F("----------------------------------"));
  Serial.println(F(" Ulixie Stopwatch v4              "));
  Serial.println(F(" by Phons Bloemen                 "));
  Serial.println(F(" Released under the GPLv3 License "));
  Serial.println(F("----------------------------------"));
}
