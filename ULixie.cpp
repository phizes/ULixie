/*
  ULixie.cpp - Library for controlling Lixies and their cousins
  
  Created by Phons Bloemen (sep 30 2020)
  For use with DIAMEX lixies.
  
  These have a led layout that differ from the 'original' lixie.
  Original Lixie-I: 10 panes, 2led/pane, serial (20 leds per lixie in the string).
  Original Lixie-II: 11 panes, 2led/pane, serial (22 leds per lixie in the string).
  DIAMEX lixie: 10 panes, 2leds/lixie, parallel (10 leds per lixie in the string).
  Elekstube lixie: UNKNOWN
  glixie lixie: UNKNOWN
  
  Based on Lixie_II library by Connor Nishijma July 6th, 2019
  Is partially upward compatible with it

  Released under the GPLv3 License
*/

#include "ULixie.h"

uint8_t get_size(uint32_t input);

// FastLED info for the LEDs
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define LEDS_PER_DIGIT 22
// #define PANE11

uint8_t n_digits;      // Keeps the number of displays
uint16_t n_LEDs;       // Keeps the number of LEDs based on display quantity.
CLEDController *lix_controller; // FastLED 
CRGB *lix_leds;
bool *special_panes_enabled;

// Standard Lixie Settings

uint8_t leds_per_digit = 22;
uint8_t reverse_string = 0;

uint8_t led_assignments[LEDS_PER_DIGIT] = { 1, 9, 4, 6, 255, 7, 3, 0, 2, 8, 5, 5, 8, 2, 0, 3, 7, 255, 6, 4, 9, 1  }; // 255 is extra pane
uint8_t x_offsets[LEDS_PER_DIGIT] = { 0, 0, 0, 0, 1,   1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4,   5, 5, 5, 5  };

uint8_t led_assignments_diamex[LEDS_PER_DIGIT] = { 1, 3, 5, 7, 9, 0, 2, 4, 6, 8 };
uint8_t x_offsets_diamex[LEDS_PER_DIGIT] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  };

//uint32_t delay_table[20] = {10,10,10,10,10,10, 10, 10, 10, 10,
//                            14,20,28,40,56,80,112,160,224,400};
uint32_t delay_table[20] = {10,10,10,10,10,10, 10, 10, 12, 15,
                            18,22,27,33,39,47, 56, 68, 82,100};
uint8_t max_x_pos = 0;

CRGB *col_on;
CRGB *col_off;

uint8_t *led_mask_0;
uint8_t *led_mask_1;

CRGB *special_panes_color;

uint8_t *cur_digits; // register to hold current contents of lixie display
CRGB *cur_col_on;    // register to hold current ON color of lixie display
CRGB *cur_col_off;   // register to hold current OFF color of lixie display

uint8_t current_mask = 0;
//float mask_push = 1.0;
uint32_t t_lastanimate = 0;
uint32_t t_lastmaskupdate = 0;
bool mask_fade_finished = true;
bool mustnowanimate = false;

uint8_t trans_type = CROSSFADE;
uint32_t trans_time = 250;
uint8_t trans_effect = INSTANT;

bool transition_mid_point = true;

float bright = 1.0;

bool background_updates = true;

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Ticker lixie_animation;
#endif

uint16_t led_to_x_pos(uint16_t led){
  uint8_t led_digit_pos = x_offsets[led%leds_per_digit];
  
  uint8_t complete_digits = 0;
  while(led >= leds_per_digit){
    led -= leds_per_digit;
    complete_digits += 1;
  }
  
  return max_x_pos - (led_digit_pos + (complete_digits*6));
}

void animate() {
  uint32 t_animate = millis();
  float mask_fader = 1.0;
  
  if (((t_animate - t_lastanimate) < 20) && (mustnowanimate == false)) { 
     // 20ms is 50 frames per second, do nothing if this call is too soon
     return;
  }
//    t_lastanimate = t_lastanimate + 21;
  t_lastanimate = t_animate;

  switch (trans_type) {

     case CROSSFADE2:
        mask_fader = mask_fader * (float)(t_animate - t_lastmaskupdate) / trans_time;
     case DEEPFADE:
     case CROSSFADE:
        mask_fader = mask_fader * (float)(t_animate - t_lastmaskupdate) / trans_time;
     case INSTANT:
        mask_fader = mask_fader * 1;
  }
  
  if ((mask_fader > 0.99) || (trans_type == INSTANT) || 
      ((t_animate - t_lastmaskupdate) > trans_time)) {
    mask_fader = 1.0;
    mask_fade_finished = true;      
  }

  uint8_t pcb_index = 0;
  
  for(uint16_t i = 0; i < n_LEDs; i++) {
    float mask_float;
    CRGB new_col;
    uint32_t r,g,b;
    uint8_t mask_input_0 = led_mask_0[i];
    uint8_t mask_input_1 = led_mask_1[i];
    
    if(current_mask == 0) {   
       mask_float = ((mask_input_0*(1-mask_fader)) + (mask_input_1*(mask_fader)));
    } else if(current_mask == 1) {
       mask_float = ((mask_input_1*(1-mask_fader)) + (mask_input_0*(mask_fader)));
    }      
    if (trans_type == DEEPFADE) {
       if (current_mask == 0) {
          if ((mask_input_0 > 0) && (mask_input_1 == 0)) {
             if (mask_fader < 0.45) {
                 mask_float = mask_input_0 * (1.0 - (2.2 * mask_fader));
             } else {
                 mask_float = 0.0;
             }
          }
          if ((mask_input_1 > 0) && (mask_input_0 == 0)) {
             if (mask_fader > 0.55) {
                 mask_float = mask_input_1 * ((mask_fader - 0.55) * 2.2); 
             } else {
                 mask_float = 0.0;
             }
          }
        } else if (current_mask == 1) {
          if ((mask_input_1 > 0) && (mask_input_0 == 0)) {
             if (mask_fader < 0.45) {
                 mask_float = mask_input_1 * (1.0 - (2.2 * mask_fader));
             } else {
                 mask_float = 0.0;
             }
          }
          if ((mask_input_0 > 0) && (mask_input_1 == 0)) {
             if (mask_fader > 0.55) {
                 mask_float = mask_input_0 * ((mask_fader - 0.55) * 2.2); 
             } else {
                 mask_float = 0.0;
             }
          }
       }
    }
    if (mask_fade_finished == true) {
        if (current_mask == 0) {  
           mask_float = (float) mask_input_1;
           led_mask_0[i] = led_mask_1[i];
        } else if (current_mask == 1) {
           mask_float = (float) mask_input_0;
           led_mask_1[i] = led_mask_0[i];
        }         
    }
    
    r = ((col_on[i].r*mask_float) + 
                 (col_off[i].r*(255-mask_float)) * bright) / 255;
    g = ((col_on[i].g*mask_float) + 
                 (col_off[i].g*(255-mask_float)) * bright) / 255;
    b = ((col_on[i].b*mask_float) + 
                 (col_off[i].b*(255-mask_float)) * bright) / 255;
   
    new_col.r = (uint8_t) r;
    new_col.g = (uint8_t) g;
    new_col.b = (uint8_t) b;
    
/*    if ((i > 49) && (i < 60) && (trans_type != INSTANT) && (mask_fade_finished == false) &&
        ((led_mask_0[i] + led_mask_1[i])> 0) && (mask_fader >0.8)) {
       Serial.print(millis());
       Serial.print(" ");
       Serial.print(t_animate);
       Serial.print(" ");
       Serial.print(t_lastanimate);
       Serial.print(" ");
       Serial.print(t_lastmaskupdate);
       Serial.print(" i");
       Serial.print(i);
       Serial.print(" curmask ");
       Serial.print(current_mask);
       Serial.print(" fader ");
       Serial.print(mask_fader);
       Serial.print(" lm ");
       Serial.print(led_mask_0[i]);
       Serial.print("-");
       Serial.print(led_mask_1[i]);
       Serial.print(" mi ");
       Serial.print(mask_input_0);
       Serial.print("-");
       Serial.print(mask_input_1);
       Serial.print(" mf ");
       Serial.print(mask_float);
       Serial.print(" b ");
       Serial.print(bright);
       Serial.print(" cON(");
       Serial.print(col_on[i].r);
       Serial.print(" ");
       Serial.print(col_on[i].g);
       Serial.print(" ");
       Serial.print(col_on[i].b);
       Serial.print(") coff(");
       Serial.print(col_off[i].r);
       Serial.print(" ");
       Serial.print(col_off[i].g);       
       Serial.print(" ");
       Serial.print(col_off[i].b);
       Serial.print(") nwcr (");
       Serial.print(new_col.r);
       Serial.print(" ");
       Serial.print(new_col.g);
       Serial.print(" ");
       Serial.print(new_col.b);
       Serial.print(") RGB ");
       Serial.println(r+g+b);
    }       
*/    

/*   if ((i > 49) && (i < 60) && (trans_type != INSTANT) && (mask_fade_finished == false) &&
        ((led_mask_0[i] + led_mask_1[i])> 0)) {
       Serial.print(millis());       
       Serial.print(" i");
       Serial.print(i);
       Serial.print(" i");
       Serial.print(trans_type);
       Serial.print(" ");
       Serial.print(trans_effect);
       Serial.print(" ");
       Serial.print(trans_time);
       Serial.print(" fader ");
       Serial.print(mask_fader);       
       Serial.print(" curmask ");
       Serial.print(current_mask);
       Serial.print(" mi ");
       Serial.print(mask_input_0);
       Serial.print("-");
       Serial.print(mask_input_1);
       Serial.print(" mf ");
       Serial.println(mask_float);
    }       
*/        
    lix_leds[i] = new_col;  

#ifdef PANE11	
	// Check for special pane enabled for the current digit, and use its color instead if it is.
	  uint8_t digit_index = i/leds_per_digit;
	  if(special_panes_enabled[digit_index]){
		  if(pcb_index == 4){
		    lix_leds[i] = special_panes_color[digit_index*2];
		  } else if(pcb_index == 17){
		    lix_leds[i] = special_panes_color[digit_index*2+1];
		  }
	  }
    pcb_index++;
	  if(pcb_index >= leds_per_digit){
		   pcb_index = 0;
	  } 
#endif
  }

  lix_controller->showLeds(); 
  
  mustnowanimate = false;
  if (mask_fade_finished == true) {
     if (current_mask == 0) {
        current_mask = 1;
     } else { 
        current_mask = 0;
     }
   }
}

void ULixie::mask_update(){
// mask_fader = 0.0;
  
//  if(trans_type == INSTANT){
//    mask_push = 1.0;
//  }
//  else{
//    float trans_multiplier = float(trans_time) / float(1000.0);
//   mask_push = (float) 1.0 / float(50.0 * trans_multiplier);
//  }
  mask_fade_finished = false;
  t_lastmaskupdate = millis();
}


void ULixie::mask_toggle() {
//    if(current_mask == 0){
//      current_mask = 1;
//    } else if(current_mask == 1){
//       current_mask = 0;
//    }
    mask_update();
    animate();
//     wait();
}

void ULixie::transition_type(uint8_t type){
  trans_type = type;
}

void ULixie::transition_time(uint16_t ms){
  trans_time = ms;
}

void ULixie::transition_effect(uint16_t type){
  trans_effect = type;
}

void ULixie::run(){
  animate();
}

void ULixie::wait(){
  while(mask_fade_finished == false) {
    animate();
  }
}

void ULixie::start_animation(){
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  lixie_animation.attach_ms(20, animate);
#elif defined(__AVR__)  
  // TIMER 1 for interrupt frequency 50 Hz:
  cli(); // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 50 Hz increments
  OCR1A = 39999; // = 16000000 / (8 * 50) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 8 prescaler
  TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts
#endif
}

#if defined(__AVR__)  
ISR(TIMER1_COMPA_vect){
   animate();
}
#endif

void ULixie::stop_animation(){
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  lixie_animation.detach();
#endif
}

ULixie::ULixie(const uint8_t pin, uint8_t number_of_digits){
  n_LEDs = number_of_digits * leds_per_digit;
  n_digits = number_of_digits;
  max_x_pos = (number_of_digits * 6)-1;
  
  lix_leds = new CRGB[n_LEDs];  
  led_mask_0 = new uint8_t[n_LEDs];
  led_mask_1 = new uint8_t[n_LEDs];

#ifdef PANE11  
  special_panes_enabled = new bool[n_digits];
  special_panes_color = new CRGB[n_digits*2];
#endif
    
  col_on = new CRGB[n_LEDs];
  col_off = new CRGB[n_LEDs];
  
  cur_digits = new uint8_t[n_digits];
  cur_col_on = new CRGB[n_digits];
  cur_col_off= new CRGB[n_digits];
  
  for(uint16_t i = 0; i < n_LEDs; i++){
    led_mask_0[i] = 0;
    led_mask_1[i] = 0;
    
    col_on[i] = CRGB(222,222,222);
    col_off[i] = CRGB(2,6,0);
  }

#ifdef PANE11
  for(uint16_t i = 0; i < n_digits*2; i++){
  	special_panes_color[i] = CRGB(0,0,0);
  }
#endif
  
  for(uint16_t i = 0; i < n_digits; i++){
     cur_digits[i] = 0;
     cur_col_on[i] = CRGB(222,222,222);
     cur_col_off[i] = CRGB(2,6,0);
  }
  
  build_controller(pin);
}

void ULixie::setDiamex(uint8_t n_digits) {
  leds_per_digit = 10;
  reverse_string = 1;
  n_LEDs = n_digits * leds_per_digit;
  for (uint16_t i = 0; i< leds_per_digit; i++) {
     led_assignments[i] = led_assignments_diamex[i];
     x_offsets[i] = 0;
  }
  for (uint16_t i = 0; i < n_LEDs; i++) {
    led_mask_0[i] = 0;
    led_mask_1[i] = 0;
    
    col_on[i] = CRGB(111,111,111);
    col_off[i] = CRGB(6,0,3);
  }
  for(uint16_t i = 0; i < n_digits; i++){
     cur_digits[i] = 0;
     cur_col_on[i] = CRGB(111,111,111);
     cur_col_off[i] = CRGB(6,0,3);
  }
}

void ULixie::build_controller(const uint8_t pin){
  //FastLED control pin has to be defined as a constant, (not just const, it's weird) this is a hacky workaround.
  // Also, this stops you from defining non existent pins with your current board architecture
  if (pin == 0)
    lix_controller = &FastLED.addLeds<LED_TYPE, 0, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 2)
    lix_controller = &FastLED.addLeds<LED_TYPE, 2, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 4)
    lix_controller = &FastLED.addLeds<LED_TYPE, 4, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 5)
    lix_controller = &FastLED.addLeds<LED_TYPE, 5, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 12)
    lix_controller = &FastLED.addLeds<LED_TYPE, 12, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 13)
    lix_controller = &FastLED.addLeds<LED_TYPE, 13, COLOR_ORDER>(lix_leds, n_LEDs);
    //FastLED.addLeds<LED_TYPE, 13, COLOR_ORDER>(lix_leds, n_LEDs);
}

void ULixie::begin(){
  max_power(5,1200); // Default for the safety of your PC USB
  start_animation();
}

void ULixie::max_power(uint8_t V, uint16_t mA){
  FastLED.setMaxPowerInVoltsAndMilliamps(V, mA);
}

void ULixie::clear_all(){
//  if(current_mask == 0){
//    for(uint16_t i = 0; i < n_LEDs; i++){
//      led_mask_0[i] = 0;
//    }
//  }
//  else if(current_mask == 1){
//    for(uint16_t i = 0; i < n_LEDs; i++){
//      led_mask_1[i] = 0;
//    }
//  }
  for(uint16_t i = 0; i < n_digits; i++) {
     led_mask_0[i] = 0;
     led_mask_1[i] = 0;
  }
  current_mask = 0;

#ifdef PANE11
  for(uint16_t i = 0; i < n_digits*2; i++){
  	special_panes_color[i] = CRGB(0,0,0);
  }
#endif

  for(uint16_t i = 0; i < n_digits; i++){
     cur_digits[i] = 128; // fill with spaces;
  }
}

bool char_is_number(char input){
  if(input <= 57 && input >= 48) // if equal to or between ASCII '0' and '9'
    return true;
  else
    return false;
}

CRGB * ULixie::readColLayer(uint8_t layer){
  CRGB * out = new CRGB[n_digits];
  for (uint8_t i = 0; i < n_digits; i++) {
    if (layer == ON) {
         out[i] = cur_col_on[i];
    } else if (layer == OFF) {
         out[i] = cur_col_off[i];
    } else { 
         out[i] = CRGB(0,0,0);
    }
  }
  return out;
}

uint8_t * ULixie::readIntValues() {
  uint8_t * out = new uint8_t[n_digits];
  for (uint8_t i = 0; i < n_digits; i++) {
     if (cur_digits[i] < 10) {
        out[i] = cur_digits[i];
     } else if (cur_digits[i] == 128) {
        out[i] = 128;
     } else {  // digit is not number or space thus special pane
        out[i] = 255;
     }
   }
   return out;
}

// char * ULixie::readCharValues() {
String ULixie::readString() {
  char * out = new char[n_digits+1];
  out[n_digits] = char('\0'); // terminate string
  for (uint8_t i = 0; i < n_digits; i++) {
     if (cur_digits[i] < 10) {
        out[i] = char(cur_digits[i]+48);
     } else if (cur_digits[i] == 128) {
        out[i] = ' ';
     } else { // digit is not number or space thus special pane
        out[i] = '.';
     }
   }
   return out;
}

void ULixie::write(String input){
/* ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
    uint8_t * prev_digits;
    uint8_t * next_digits;
    uint32_t * delay_times;
    uint8_t * delay_index;
    uint32_t t_now, t_start;
    uint8_t i, j, num;
    uint32_t delaytime;
    uint8_t digitwritten;
    uint8_t reelsstopped;
    uint8_t oldtrans_type;
    uint32_t oldtrans_time;
    
    prev_digits = new uint8_t[n_digits];
    next_digits = new uint8_t[n_digits];
    delay_times = new uint32_t[n_digits];
    delay_index = new uint8_t[n_digits];
    
    oldtrans_type = trans_type;
    oldtrans_time = trans_time;
//    trans_type = INSTANT;
    
//    stop_animation();
    
    for (i = 0; i < n_digits; i++) {
       if (i < input.length()) {
          char c = input.charAt(i);
          if (char_is_number(c)) {
             next_digits[n_digits - i - 1] = c - '0';
          } else if (c == ' ') {
             next_digits[n_digits - i - 1] = 128;
          } else {
             next_digits[n_digits - i - 1] = 255;
          }
       } else {
          next_digits[n_digits - i - 1] = 128; //pad with spaces
       }
       prev_digits[i] = read_digit(i);
    }      
    t_start = millis();
    t_now = millis();
//    Serial.print(t_now);
//    Serial.print(" ");
//    Serial.print(trans_effect);
    if (trans_effect == SLOTMACHINE) {
/* SLOTMACHINE effect: Every lixie display is a reel on a slot machine. 
   They spin and will stop on the number indicated in the call to this 
   write function. The reel slows down like a reel slot machine, next 
   reel will slow down once previous reel stopped. 
   - trans_time will indicate the spinning speed.
*/
//       Serial.println(" SLOTMACHINE");
       clear_all();
       for (i=0; i < n_digits; i++) {
          if (next_digits[i] < 10) { // only spin reels with a symbol
             delay_index[i] = 10;
             delay_times[i] = t_now;
          } else {
             delay_index[i] = 19;
             delay_times[i] = 0;
             write_digit(i,128);
          }
       }
       reelsstopped = 0;
       while (reelsstopped < n_digits) {
          t_now = millis();
          if ((t_now % 20) == 0) {
             for (i = 0; i < n_digits; i++) {
                digitwritten = 0;
                if (delay_index[i] < 20) {
                   for (j = i; j < n_digits; j++) {
                      if ((t_now - delay_times[j]) > 
                           (trans_time * delay_table[delay_index[j]] / 10)) {
                         if (j == i) {
                            delay_index[j] += 1;
                         }
                         num = (read_digit(j) + random(9)) % 10;
                         if (next_digits[j] > 10) num = 128;
                         write_digit(j,num);
                         digitwritten++;
                         delay_times[j] = t_now;
                      }
                   }
//                      Serial.print(i);
//                      Serial.print(" ");
//                      Serial.print(k);
//                      Serial.print(" ");
//                      Serial.print(delay_index[i]);
//                      Serial.print(" ");
//                      Serial.print(num);
//                      Serial.print(" ");
//                      Serial.print(next_digits[i]);
//                      Serial.print(" ");
//                      Serial.println(t_now);
                }
                if ((delay_index[i] > 19) && (delay_times[i] > 0)) {
                   reelsstopped = i + 1;
                   delay_times[i] = 0;
//                   Serial.print("REEL ");
//                   Serial.print(k);
//                   Serial.print(" [");
//                   Serial.print(readString());
//                   Serial.print("] ");
//                   Serial.println(t_now);
                   num = next_digits[i];
                   write_digit(i,num);
                   digitwritten++;
                }
                if (digitwritten > 0) mask_toggle();
             }      
             animate();
             yield();
          } 
       }     
    } else if (trans_effect == SLOTIIMACHINE) {
/* SLOTIIMACHINE effect: A variant on the slotmachine theme. 
   Works like SLOTMACHINE, except next reel will slow down 
   before preivious reel stops. 
   - trans time indicates the spinning speed
*/
//       Serial.println(" SLOTIIMACHINE");
       
       clear_all();
       for (i=0; i < n_digits; i++) {
          if (next_digits[i] < 10) { // only spin reels with a symbol
             delay_index[i] = 2 * i;
             if (delay_index[i] > 16) delay_index[i] = 16;
             delay_times[i] = t_now - i;
          } else {
             delay_index[i] = 19;
             delay_times[i] = 0;
             write_digit(i,128);
          }
       }
       reelsstopped = 0;
       while (reelsstopped < n_digits) {
          t_now = millis();
          if (t_now % 20 == 0) {         
             digitwritten = 0;
             for (i = 0; i < n_digits; i++) {
                if (delay_index[i] < 20) {
                   if ((t_now - delay_times[i]) > 
                        (trans_time * delay_table[delay_index[i]]) / 10) {
                      num = read_digit(i) + random(9) % 10;
                      if (next_digits[i] > 10) num = 128;
                      delay_index[i] = delay_index[i] + 1;
                      delay_times[i] = t_now;
//                      Serial.print(i);
//                      Serial.print(" ");
//                      Serial.print(k);
//                      Serial.print(" ");
//                      Serial.print(delay_index[i]);
//                      Serial.print(" ");
//                      Serial.print(num);
//                      Serial.print(" ");
//                      Serial.print(next_digits[i]);
//                      Serial.print(" ");
//                      Serial.println(t_now);
                      if (delay_index[i] > 19) {  // table has 20 entries
                         reelsstopped++;
//                         Serial.print("REEL ");
//                         Serial.print(k);
//                         Serial.print(" [");
//                         Serial.print(readString());
//                         Serial.print("] ");
//                         Serial.println(t_now);
                         num = next_digits[i];
                      }
                      write_digit(i,num);
                      digitwritten++;
                   }
                }
             }
             if (digitwritten > 0) mask_toggle();
//              animate();      // --> mask_toggle();
             yield();
          }
       }
    } else if (trans_effect == GASPUMP) {
/* GASPUMP effect: Like an old gaspump with electromechanical 'flappy' 
   7segnment displays. At start, all lixies that must display a digit 
   will spin in unison (all numbers equal) After one full cycle, every 
   next cycle the next display will stop at its intended value while 
   the rest keeps spinning.
   - trans_time will indicate the spinning speed.
*/  
//       Serial.println(" GASPUMP");
       clear_all();
       for (i=0; i < n_digits; i++) {
          if (next_digits[i] < 10) { // only spin reels with a symbol
             delay_index[i] = 1;
             delay_times[i] = t_now - i;
          } else {
             delay_index[i] = 19;
             delay_times[i] = 0;
          }
       }
       num = 0;
       reelsstopped = 0;
       t_now = millis() + trans_time + 1;
       while (reelsstopped < n_digits) {
          if ((t_now - delay_times[reelsstopped]) > trans_time) {
             digitwritten = 0;
             for (i = 0; i < n_digits; i++) {
                if (delay_index[i] < 20) {
                   delay_times[i] = t_now;
                   if (i == reelsstopped) {
                      delay_index[i] = delay_index[i] + 1;                      
                      if (delay_index[i] > 10) {
//                   Serial.print(i);
//                   Serial.print(" ");
//                   Serial.print(reelsstopped);
//                   Serial.print(" ");
//                   Serial.print(delay_index[i]);
//                   Serial.print(" ");
//                   Serial.print(num);
//                   Serial.print(" ");
//                   Serial.print(next_digits[i]);
//                   Serial.print(" ");
//                   Serial.println(t_now);
                         if ((next_digits[i] == num) || 
                             (next_digits[i] > 10)) {
                            delay_index[i] = 20;
                            reelsstopped++;
//                            Serial.print("REEL ");
//                            Serial.print(reelsstopped);
//                            Serial.print(" [");
//                            Serial.print(readString());
//                            Serial.print("] ");
//                            Serial.println(t_now);
                         }
                      }
                   }
                   if (next_digits[i] > 10) {
                      write_digit(i,128);
                   } else {
                      write_digit(i,num);
                   }
                   digitwritten++;
                }
             }
             if (digitwritten > 0) mask_toggle();
             num = (num + 1) % 10;
          }
          if (t_now % 20 == 0) {
             animate();
             yield();
          }
          t_now = millis();         
       }
    } else if (trans_effect == PINBALL) {
/* PINBALL effect: Like an old pinballmachine with electromechanical 
   score displays. In the first round, the previous value of the lixie 
   is taken into acount. They start spining to zero when the counter is 
   equal to their value (like the score reset on the pinball machine). 
   Then lixies that do not need to display a number are cleared an the 
   rest will take 2 complete 0-9 spin rounds in unison. In the third 
   round the lixies will stuck on their intended value. 
   - trans_time will indicate the spinning speed.
*/
//       Serial.println(" PINBALL");
       clear_all();
       for (i=0; i < n_digits; i++) {
          if (next_digits[i] < 10) { // only spin reels with a symbol
             delay_index[i] = 1;
             delay_times[i] = t_now - i;
          } else {
             delay_index[i] = 1;
             delay_times[i] = 0;
          }
       }
       reelsstopped = 0;
       num = 0;
       t_now = millis() + trans_time + 1;
       while (reelsstopped < n_digits) {
          if ((t_now - delay_times[reelsstopped]) > trans_time) {
             digitwritten = 0;
             for (i = 0; i < n_digits; i++) {
                if (delay_index[i] < 30) {
                   delay_times[i] = t_now;
                   delay_index[i] = delay_index[i] + 1;
//                   Serial.print(i);
//                   Serial.print(" ");
//                   Serial.print(reelsstopped);
//                   Serial.print(" ");
//                   Serial.print(delay_index[i]);
//                   Serial.print(" ");
//                   Serial.print(num);
//                   Serial.print(" ");
//                   Serial.print(next_digits[i]);
//                   Serial.print(" ");
//                   Serial.println(t_now);
                   if (next_digits[i] > 10) {
                      write_digit(i,128);
                   } else {                   
                      write_digit(i,num);
                   }
                   digitwritten++;
                   if (delay_index[i] > 20) {
                      if ((next_digits[i] == num) || 
                          (next_digits[i] > 10)) {
                         delay_index[i] = 30;
                         reelsstopped++;
//                         Serial.print("REEL ");
//                         Serial.print(reelsstopped);
//                         Serial.print(" [");
//                         Serial.print(readString());
//                         Serial.print("] ");
//                         Serial.println(t_now);
                      }
                   }
                }
             }
             if (digitwritten > 0) mask_toggle();
             num = (num + 1) % 10;
          }
          if (t_now % 20 == 0) {
             animate(); 
             yield();
          }
          t_now = millis();         
       }
    } else if (trans_effect == ONEBYONELEFT) {
/* ONEBYONELEFT effect: The new value will be 'dropped' in from left  to right. 
   thereby overwriting the previous indicated values. 
   - trans_time will indicate the speed from left to right.
*/        
//       Serial.println(" ONEBYONELEFT");
       i = 0;
       while (i < n_digits) {
          if ((t_now - delaytime) > trans_time) {
             write_digit(n_digits - 1 - i, next_digits[n_digits - 1 - i]);
             delaytime = t_now;
             mask_toggle();
             i++;
          }
          if (t_now % 20 == 0) {
             animate();
             yield();
          }
          t_now = millis();         
       }      
    } else if (trans_effect == ONEBYONERIGHT) {
/* SCROLLEFT effect: The new value will be 'dropped' in from right to left, 
   thereby overwriting the previous indicated values. 
   - trans_time will indicate the dropping speed.
*/     
//       Serial.println(" ONEBYONERIGHT");
       i = 0;
       while (i < n_digits) {
          if ((t_now - delaytime) > trans_time) {
             write_digit(i, next_digits[i]);
             delaytime = t_now;
             mask_toggle();
             i++;
          }
          if (t_now % 20 == 0) {
             animate();
             yield();
          }
          t_now = millis();         
       }
    } else if (trans_effect == SCROLLLEFT) {
/* SCROLLEFT effect: The new value will be 'pushed' in from the left side, 
   thereby 'pushing' forward the previous indicated values. 
   - trans_time will indicate the pushing speed.
*/        
//       Serial.println(" SCROLLEFT");
       reelsstopped = 0;
       num = 0;
       delaytime = 0;
       t_now = millis() + trans_time + 1;
       while (reelsstopped < n_digits + 1) {
          if ((t_now - delaytime) > trans_time) {
             digitwritten = 0;
             for (i = 0; i < (n_digits); i++) {
                j = i + reelsstopped;
                if (j < (n_digits)) {
                   write_digit(i,prev_digits[j]);
                } else {
                   j = j - n_digits;
                   write_digit(i,next_digits[j]);
                } 
                digitwritten++;
                delaytime = t_now;
             }
             if (digitwritten > 0) mask_toggle();
             reelsstopped++;
          }
          if (t_now % 20 == 0) {
             animate();
             yield();
          }
          t_now = millis();         
       }
    } else if (trans_effect == SCROLLRIGHT) {
/* SCROLLRIGHT effect: The new value will be 'pushed' in from the right 
   side, thereby 'pushing' forward the previous indicated values. 
   - trans_time will indicate the pushing speed.
*/        
//       Serial.println(" SCROLLRIGHT");
       reelsstopped = 0;
       num = 0;
       delaytime = 0;
       t_now = millis() + trans_time + 1;
       while (reelsstopped < n_digits + 1) {
          if ((t_now - delaytime) > trans_time) {
             digitwritten = 0;
             for (i = 0; i < (n_digits); i++) {
                j = i + reelsstopped;
                if (j < (n_digits)) {
                   write_digit((n_digits -1)-i,prev_digits[(n_digits-1)- j]);
                } else {
                   j = j - n_digits;
                   write_digit((n_digits -1)-i,next_digits[(n_digits-1)- j]);
                } 
                digitwritten++;
                delaytime = t_now;
             }
             if (digitwritten > 0) mask_toggle();
             reelsstopped++;
          }
          if (t_now % 20 == 0) {
             animate();
             yield();
          }
          t_now = millis();         
       }
    } else if (trans_effect == SCROLLFLAPLEFT) {
/*  SCROLLFLAPLEFT effect: Every lixie will 'flap' from its previous 
    indicated value to its intended indicated value, theY will flap at 
    least one complete 0-9 round. They will change one by one from left 
    to right.
    - trans_time indicates the flapping speed.      
*/
//       Serial.println(" SCROLLFLAPLEFT");
       for (i=0; i < n_digits; i++) {
          delay_index[i] = 1;
          delay_times[i] = t_now - i;
       }
       reelsstopped = 0;
       num = 0;
       t_now = millis() + trans_time + 1;
       while (reelsstopped < n_digits) {
          if ((t_now - delay_times[reelsstopped]) > trans_time) {
             digitwritten = 0;
             for (i = 0; i < (n_digits); i++) {
                num = read_digit(i);
                if (i == reelsstopped) {
                   if (delay_index[i] < 20) {
                      delay_times[i] = t_now;
                      if (num > 127) { 
                         num = 9;
                      }
                      num = (num + 1) % 10;
                      delay_index[i] = delay_index[i] + 1;                      
                      if (delay_index[i] > 10) {
                         if (next_digits[i] == num) {
                            delay_index[i] = 20;
                            reelsstopped++;
//                            Serial.print("REEL ");
//                            Serial.print(k);
//                            Serial.print(" [");
//                            Serial.print(readString());
//                            Serial.print("] ");
//                            Serial.println(t_now);
                         }
                         if ((next_digits[i] == 128) && (num == 0)) {
                            delay_index[i] = 20;
                            reelsstopped++;
                            num = 128;
//                            Serial.print("REEL ");
//                            Serial.print(k);
//                            Serial.print(" [");
//                            Serial.print(readString());
//                            Serial.print("] ");
//                            Serial.println(t_now);
                         }
                         if ((next_digits[i] == 255) && (num == 0)) {
                            delay_index[i] = 20;
                            reelsstopped++;
                            num = 255;
                         }
                      }
                   }
                }
                write_digit(i,num);
                digitwritten++;
             }
             if (digitwritten > 0) mask_toggle();
          }
          if (t_now % 20 == 0) {
             animate();
             yield();
          }
          t_now = millis();         
       }
    } else if (trans_effect == SCROLLFLAPRIGHT) {
/*  SCROLLFLAPRIGHT effect: Every lixie will 'flap' from its previous 
    indicated value to its intended indicated value, they will flap at 
    least one complete 0-9 round. They will change one by one from right 
    to left.
    - trans_time indicates the flapping speed.      
*/
//       Serial.println(" SCROLLFLAPRIGHT");
       for (i=0; i < n_digits; i++) {
          delay_index[i] = 1;
          delay_times[i] = t_now - i;
       }
       reelsstopped = 0;
       num = 0;
       t_now = millis() + trans_time + 1;
       while (reelsstopped < n_digits) {
          if ((t_now - delay_times[reelsstopped]) > trans_time) {
             digitwritten=0;
             for (i = 0; i < (n_digits); i++) {
                num = read_digit((n_digits - 1) - i);
                if (i == reelsstopped) {
                   if (delay_index[i] < 20) {
                      delay_times[i] = t_now;
                      if (num > 127) { 
                         num = 9;
                      }
                      num = (num + 1) % 10;
                      delay_index[i] = delay_index[i] + 1;                      
                      if (delay_index[i] > 10) {
                         if (next_digits[(n_digits - 1) -i] == num) {
                            delay_index[i] = 20;
                            reelsstopped++;
//                            Serial.print("REEL ");
//                            Serial.print(k);
//                            Serial.print(" [");
//                            Serial.print(readString());
//                            Serial.print("] ");
//                            Serial.println(t_now);
                         }
                         if ((next_digits[(n_digits -1) - i] == 128) && (num == 0)) {
                            delay_index[i] = 20;
                            reelsstopped++;
                            num = 128;
//                            Serial.print("REEL ");
//                            Serial.print(k);
//                            Serial.print(" [");
//                            Serial.print(readString());
//                            Serial.print("] ");
//                            Serial.println(t_now);
                         }
                         if ((next_digits[(n_digits - 1) - i] == 255) && (num == 0)) {
                            delay_index[i] = 20;
                            reelsstopped++;
                            num = 255;
                         }
                      }
                   }
                }
                write_digit((n_digits - 1) -i,num);
                digitwritten++;
             }
             if (digitwritten > 0) mask_toggle();
          }
          if (t_now % 20 == 0) {
             animate();
             yield();
          }
          t_now = millis();         
       }
    } else if (trans_effect == BLINKER) {
/* BLINKER effect: after the intended value is written just like that 
   (like 'INSTANT'), the display will blink 10 times. This blinking part 
   of the write action.
   - trans_time will indicate the blinking speed.
*/
//       Serial.println(" BLINKER");
       delaytime = t_now;
       j = 0;
       num = 0;
       t_now = millis() + trans_time + 1;
       while (j < 19) {
          if ((t_now - delaytime) > trans_time) {
             digitwritten=0;
             for (i = 0; i < (n_digits); i++) {
                if ((j % 2) == 0) {
                   num = next_digits[i];
                } else { 
                   num = ' ';
                }
                write_digit(i,num);
                digitwritten++;
             }
             if (digitwritten > 0) mask_toggle();
             delaytime = t_now;
             j++;
          }   
          if (t_now % 20 == 0) {
             animate();
             yield();
          }
          t_now = millis();         
       }
    } else {  // trans_ffect must be INSTANT
/*  INSTANT effect: The intended value is written to the display in one go 
    without any other effects. This effect is used a default when the 
    trans_effect variable contains a non described effect number. 
*/
//     clear_all(); --> colors must be set befor call to write
//       Serial.println(" INSTANT");
       trans_type = oldtrans_type;
       trans_time = oldtrans_time;
       for (i = 0; i < n_digits; i++) {
          write_digit(i, next_digits[i]);
       }
       mask_toggle();
       yield();
       // no delay whatsever
    }
//    Serial.print(trans_effect);
//    Serial.print(" ");
//    Serial.print(t_now);
//    Serial.print(" ");
//    Serial.print(t_start);
//    Serial.print(" [");
//    Serial.print(input);
//    Serial.println("]");

//    start_animation();
 
    trans_type = oldtrans_type;
    trans_time = oldtrans_time;
    free(prev_digits);
    free(next_digits);
    free(delay_times);
    free(delay_index);
}

void ULixie::write(uint32_t input) {
     int32_t i = (int32_t) input;
     write(i);
}

void ULixie::write(int32_t input){
/*
 *  Write() can handle positive numbers and negative numbers.
 *  If a negative number is passed, the result will be prepended with an extra zero
 *  to denote the minus sign.
 *  if the value is 'out of bounds' the value is passed to the floatplus()
 *  function (which can handle bigger absolute values).
 *  If (left side) padding is needed, it will be with spaces.
 */
    char * next_digits;
    uint8_t i, z;
    int32_t n_place, absinput;
    char num; 
    
    next_digits = new char[n_digits];
    
    z = 0;
    absinput = input;
    if (input < 0) absinput = (0 - input);

//  if numbers are too big pass them to write_floatplus
    if (input < -99999) {
       write_floatplus((float) input, get_color_display(0,ON));
       return;
    }
    if (input > 999999) {
       write_floatplus((float) input, get_color_display(0,ON));
       return;
    }
    n_place = 100000;
    if (absinput == 0) {
       write("     0");
    } else {
      for (i = 0; i < n_digits; i++) {
         num = (char)(absinput / n_place) + 48;
         if ((z == i) && (num == 48)) {
            num = ' ';
            z++;
         }  else if (input < 0) {
            if ((z > 0) && (z == i)) {
               next_digits[i - 1] = '0';
            } else {
               // this should only happen if input between -10000 and -99999
            }
         }
         next_digits[i] = num;
         absinput = absinput % n_place;
         n_place = n_place / 10;
       }
       write(next_digits);
    }
    free(next_digits);
}

void ULixie::write_float(float input_raw, uint8_t dec_places) {
  char * next_digits;
  uint8_t comma = 0, i = 0;
    
  next_digits = new char[n_digits];
  char * buf = new char[20];
  sprintf(buf,"%6.4f",input_raw);
  comma = 0;
  for (i = 0; i < n_digits; i++) {
    next_digits[i] = ' ';
  }
  for (i = 0; i < n_digits; i++) {
    if ((buf[i] > 47 ) && (buf[i] < 58)) {
       for (uint8_t j = 0; j < n_digits -1; j++) {
         next_digits[j] = next_digits[j+1];
       }
       next_digits[n_digits - 1] = buf[i];
       if (comma == i) comma++;
       if ((i - comma) > (dec_places - 1)) break;
    } else if (buf[i] == '.' ) {
       if (comma == 0) {
          // due to formatting the comma cannot be in position zero.
       }
       if (comma > (n_digits-2)) {
          // special case when the comma would be in last position. 
          // the front lixie will be cleared.
          break;
       } else {
          for (uint8_t j = 0; j < n_digits -1; j++) {
             next_digits[j] = next_digits[j+1];
          }
          next_digits[n_digits - 1] = ' ';
       }
    } else if (buf[i] == 'e' ) {
       // do nothing, an exception, ignore the exponent for so long       
       break;
    } 
  }
  write(next_digits);
  free(next_digits);
  free(buf);
}   

void ULixie::write_floatplus(float input_raw, CRGB col){
/*  
  Write_floatplus() displays the input as follows:
  Note: this assumes there is no special pane containing a '.';
  Also it uses an extra leading zero for the minus sign.
  If (left side) padding is needed it wil be spaces.
  
  if input between 0 and 1e-09: ERROR. display is "     0".
  if input between 1e-09 and 0.000001: leading zero, 3 significant in dimmed color, 
                                       a zero and the (single digit) exponent 
  if input between 0.000001 and 1: leading zero and 5 decimals in dimmed color.
  if input between 1 and 10: leading digit plus 5 decimals in dimmed color.
  if input between 10 and 100: 2 leading digits plus 4 decimals in dimmed color.
  ...
  if input between 10000 and 100000: 5 leading digits plus one decimal in dimmed color.
  if input between 100000 and 1000000: 6 leading digits.
  if input between 1e06 and 1e10: one leading digit, next 4 leading digits in dimmed color
                                one digits for the exponent in the normal color.
  if input between 1e10 and 1e100: one leading digit, next 3 leading digits in dimmed color
                                two digits for the exponen in the normal color.
  if input bigger than 1e100 (a googol): ERROR. 
                                Display "999999" with the 9s in position 2-3-4 dimmed
  if input == 0: displays ("000000") with positions 2-3-4-5-6 dimmed
  if input between 0 and -1e-09: ERROR. display is "     0".
  if input between -1e-09 and -1e-04: 2 zeros, 2 dimmed significant decimals, 
                                      a zero and the (single digit) negative exponent 
  if input between -0.0001 and -1: 2 zeros plus 4 dimmed decimals.
  if input between -1 and -10: a zero, the leading digit plus 4 dimmed decimals.
  if input between -10 and -100: a zero, 2 leading digits plus 3 dimmed decimals.
  ...
  if input between -1000 and -10000: a zero, 4 leading digits plus 1 dimmed decimal.
  if input between -10000 and -100000: a zero and 5 leading digits.
  if input between -1e05 and -1e10: a zero, one leading digit, 3 dimmed decimals,
                                one digits for the exponent (not dimmed).
  if input between -1e10 and -1e100: a zero, one leading digit, 2 dimmed decimals,
                                two digits for the exponent (not dimmed).
  if inptu smaller than -1e100 (a googol): ERROR. 
                                Display "099999" with the 9s in position 3-4 dimmed
  if input == 0: 
  if input between -0.000001 and -1:

  The argumment color is chosen for the color the lixies will light up. If a lixie is to be dimmed, 
  (r,g,b) values of the color are divided by 4.
*/

  char * next_digits;
  uint16_t dec_places = 5, digitspushed = 0;
  uint8_t comma = 0, i = 0, exponent = 0;
  
  next_digits = new char[n_digits+1];
  char * buf = new char[20];

  for (uint8_t i = 0; i<20; i++) buf[i] = ':';
  for (uint8_t i = 0; i < n_digits; i++) next_digits[i] = ' ';
  next_digits[n_digits] = '\0';
  sprintf(buf,"%6.5f",input_raw);

  // error conditions
  if (input_raw > 1000000) sprintf(buf,"%4.3e",input_raw);
  if (input_raw < -100000) sprintf(buf,"%4.3e",input_raw); 
  if ((input_raw > 0) && (input_raw < 0.001)) sprintf(buf,"%4.3e",input_raw); 
  if ((input_raw < 0) && (input_raw > -0.01)) sprintf(buf,"%4.3e",input_raw);
  if ((input_raw > 1e34) ||
      (input_raw < -1e34) ||
      ((input_raw > 0) && (input_raw < 1e-09)) ||
      ((input_raw < 0) && (input_raw > -1e-09)) ||
      (buf[1] > 57)) {
         // error conditions, if buf now contains 'inf' or -inf' that is also an error
         for (uint8_t j = 0; j < n_digits; j++) {
            next_digits[j] = '9';
            if ((j == 2) || (j == 3)) {
               color_display(j,ON,col);
            } else {
               color_display(j,ON,CRGB(col.b/4,col.g/4,col.r/4));
            }
         }
         write(next_digits);
         free(buf);
         free(next_digits);
         return;  
  }
  comma = 0;
  digitspushed = 0;
  for (i = 0; i < strlen(buf); i++) {
    switch (buf[i]) {
    case ' ' :
       break;
    case '+' :
       if (exponent == 1) {
          // there may be a + sign after the exponent sign. discard it.
       } else if (exponent == 0) {
          // ther may be a plus sign as first character, should not happen
       }
       break;
    case '-' :
       if (digitspushed < (n_digits)) {
          if ((digitspushed - comma) < dec_places) {
             for (uint8_t j = 0; j < n_digits -1; j++) {
                next_digits[j] = next_digits[j+1];
             }
             next_digits[n_digits - 1] = buf[i];
             digitspushed++;
             if (comma == i) {
                comma++;
             }
          }
       }
          // first minus sign for negative values ******
          // prepend a zero to denote minus
          // this zero will always be the first number in the output
       if (exponent == 0) {
           next_digits[n_digits - 1] = '0';
       } else if (exponent == 1) {      
           exponent = 2;    
           next_digits[n_digits - 1] = '0';  
       } 
       break;
    case '.' :
       if (comma == 0) {
          break;
          // this should not happen, due to the formatting comma cannot in pistion zero
       }
       if (comma > (n_digits-1)) {
          break;          // do not push anything if comma would be in last postion
       }
       if (comma > (n_digits-2)) {
          // special case when the comma would be in last position.
          // we can now squeeze an extra decimal in 
       }
       break;
    case 'e' :
       // exponent character.;
       exponent = 1;
       break;
    case '0' :
       if (exponent > 0) {
          // drop this leading zero for the exponent
          break;
       }
    default  :
       if ((buf[i] > 47 ) && (buf[i] < 58)) {
          // numbers are pushed into the output
           if (digitspushed < (n_digits)) {
              if ((digitspushed - comma) < dec_places) {
                 for (uint8_t j = 0; j < n_digits -1; j++) {
                   next_digits[j] = next_digits[j+1];
                 }
                 next_digits[n_digits - 1] = buf[i];
                 digitspushed++;
                 if (comma == i) {
                    comma++;
                 }
                 if (exponent > 0) {
                    exponent++;
                 }
              }
           } else if (exponent > 1) {
              // exponent of 2 digits, hard on te last two lixies (pos. 0 and 1).
              next_digits[n_digits - 2] = next_digits[n_digits -1];
              next_digits[n_digits - 1] = buf[i];
              exponent++;
           } else if ((exponent == 1) && (buf[i] > 48)) {
              // exponent of 1 digit, hard on the last lixie (only if not a zero).
              next_digits[n_digits - 1] = buf[i];
              exponent++; 
           }
        }
        // anything else (like spaces) is discarded
     }
  }
  for (uint8_t j = 0; j < n_digits; j++) {
    if ((j+1 < exponent) || ((j+1) > (digitspushed - comma))) {
       color_display(j,ON,col);
    } else {
       color_display(j,ON,CRGB(col.b/4,col.g/4,col.r/4));
    }
  }  
  write(next_digits);
  free(buf);
  free(next_digits);
}   
  
void ULixie::push_digit(uint8_t number) {
  // 0-9 are rendered normally when passed in, 
  // but 128 = blank display & 255 = special pane	
  // If multiple displays, move all LED states forward one
  
if (reverse_string == 1) {
  if (n_digits > 1) {
    for (uint16_t i = 0; i < (n_LEDs - leds_per_digit); i++) {
      if(current_mask == 0){
        led_mask_0[i] = led_mask_0[i + leds_per_digit];
      } else{
        led_mask_1[i] = led_mask_1[i + leds_per_digit];
      }
    }
  }
  // Clear the LED states for the first display
  for (uint16_t i = 0; i < leds_per_digit; i++) {
    if(current_mask == 0){
      led_mask_0[(n_LEDs-1) - i] = led_mask_0[(n_LEDs-leds_per_digit-1)- i];
    } else {
      led_mask_1[(n_LEDs-1) - i] = led_mask_1[(n_LEDs-leds_per_digit-1) - i];
    }
  }  
  if(number != 128){
    for(uint8_t i = 0; i < leds_per_digit; i++){
      if(led_assignments[i] == number){
        if(current_mask == 0){
          led_mask_0[(n_LEDs - leds_per_digit) + i] = 255;
        } else {
          led_mask_1[(n_LEDs - leds_per_digit) + i] = 255;
        }
      }	else {
        if(current_mask == 0){
          led_mask_0[(n_LEDs - leds_per_digit) + i] = 0;
        } else {
          led_mask_1[(n_LEDs - leds_per_digit) + i] = 0;
        }
      }
    }
  } else {
    for(uint8_t i = 0; i < leds_per_digit; i++){
      if(current_mask == 0){
        led_mask_0[(n_LEDs - leds_per_digit) + i] = 0;
      } else {
        led_mask_1[(n_LEDs - leds_per_digit) + i] = 0;
      }
    }
  }
  for (uint8_t i = 0; i < n_digits - 1; i++) {
    cur_digits[i] = cur_digits[i+1];
    cur_col_on[i] = cur_col_on[i+1];
    cur_col_off[i] = cur_col_off[i+1];
  }
  cur_digits[n_digits-1] = number; 
  cur_col_on[n_digits-1] = cur_col_on[n_digits -2];
  cur_col_off[n_digits-1] = cur_col_off[n_digits -2];
   
} else { // reverse_string = 0.....

  // 0-9 are rendered normally when passed in, 
  // but 128 = blank display & 255 = special pane	
  // If multiple displays, move all LED states forward one
  if (n_digits > 1) {
    for (uint16_t i = n_LEDs - 1; i >= leds_per_digit; i--) {
      if(current_mask == 0){
        led_mask_0[i] = led_mask_0[i - leds_per_digit];
      } else{
        led_mask_1[i] = led_mask_1[i - leds_per_digit];
      }
    }
  }  
  // Clear the LED states for the first display
  for (uint16_t i = 0; i < leds_per_digit; i++) {
    if(current_mask == 0){
      led_mask_0[i] = led_mask_0[i - leds_per_digit];
    } else {
      led_mask_1[i] = led_mask_1[i - leds_per_digit];
    }
  }
  if(number != 128){
    for(uint8_t i = 0; i < leds_per_digit; i++){
      if(led_assignments[i] == number){
        if(current_mask == 0){
          led_mask_0[i] = 255;
        } else {
          led_mask_1[i] = 255;
        }
      }	else {
        if(current_mask == 0){
          led_mask_0[i] = 0;
        } else {
          led_mask_1[i] = 0;
        }
      }
    }
  } else {
    for(uint8_t i = 0; i < leds_per_digit; i++){
      if(current_mask == 0){
        led_mask_0[i] = 0;
      } else {
        led_mask_1[i] = 0;
      }
    }
  }
  for (uint8_t i = n_digits -1; i > 0; i--) {
     cur_digits[i] = cur_digits[i-1];
     cur_col_on[i] = cur_col_on[i-1];
     cur_col_off[i] = cur_col_off[i-1];
  }
  cur_digits[0] = number;
  cur_col_on[0] = cur_col_on[1];
  cur_col_off[0] = cur_col_off[1];

// 
} // endif for if (reverse_string == 0)
}

uint8_t ULixie::read_digit(uint8_t num) {
     if (cur_digits[num] < 10) {
        return cur_digits[num];
     } else if (cur_digits[num] == 128) {
        return 128;
     } else {  // digit is not number or space thus special pane
        return 255;
     }
}

/* char ULixie::read_digitchar(uint8_t num) {
     if (cur_digits[num] < 10) {
        return cur_digits[num] + 48 ;
     } else if (cur_digits[num] == 128) {
        return ' ';
     } else {  // digit is not number or space thus special pane
        return '.';
     }
}
*/

void ULixie::write_digit(uint8_t digit, uint8_t num){
  uint16_t start_index = leds_per_digit*digit;
  if (!(digit < n_digits)) {
     Serial.println("Digit more than lixies");
     return;
  }
  if (reverse_string == 1) {
     start_index = ((n_digits - digit) - 1) * leds_per_digit;;  
  }  
//  clear_digit((n_digits - digit) - 1);
//  for (uint16_t i = 0; i < leds_per_digit; i++) {
//    if(current_mask == 0){
//      led_mask_0[i + start_index] = led_mask_0[i + start_index - leds_per_digit];
//    } else {
//      led_mask_1[i + start_index] = led_mask_1[i + start_index - leds_per_digit];
//    }
//  }  
//  for(uint16_t i = 0; i < leds_per_digit; i++){
//    if(current_mask == 0){
//      led_mask_1[start_index+i] = 0;//CRGB(0*0.06,100*0.06,255*0.06);   
//    }
//    if(current_mask == 1){
//      led_mask_0[start_index+i] = 0;//CRGB(0*0.06,100*0.06,255*0.06);   
//    }
//  }
//  mask_update();

//  for (uint16_t i = 0; i < leds_per_digit; i++) {
//     led_mask_0[i + start_index] = 0;
//     led_mask_1[i + start_index] = 0;
//  }

  for(uint8_t i = 0; i < leds_per_digit; i++){
     led_mask_0[i+start_index] = 0;
     led_mask_1[i+start_index] = 0;
     if (current_mask == 1) {
        if (led_assignments[i] == cur_digits[digit]) {
           if (trans_type != INSTANT) {
              led_mask_1[i+start_index] = 255;
           }
        }
        if (num != 128) {
           if (led_assignments[i] == num) {
              led_mask_0[i+start_index] = 255;
           }
        }
     } else {
        if (led_assignments[i] == cur_digits[digit]) {
           if (trans_type != INSTANT) {
              led_mask_0[i+start_index] = 255;
           }
        }
        if (num != 128) {
           if (led_assignments[i] == num) {
              led_mask_1[i+start_index] = 255;
           }
        }
     }
  }
  cur_digits[digit] = num;
}

void ULixie::clear_digit(uint8_t digit, uint8_t num){
// should guard againt too large a value for 'num'
  uint16_t start_index = leds_per_digit*digit;
  if (reverse_string == 1) {
     start_index = ((n_digits - digit) - 1) * leds_per_digit;;  
  }
  for(uint8_t i = 0; i < leds_per_digit; i++){
     led_mask_0[i+start_index] = 0;
     led_mask_1[i+start_index] = 0;
     if (current_mask == 1) {
        if (led_assignments[i] == cur_digits[digit]) {
           if (trans_type != INSTANT) {
              led_mask_1[i+start_index] = 255;
           }
        }
     } else {
        if (led_assignments[i] == cur_digits[digit]) {
           if (trans_type != INSTANT) {
              led_mask_0[i+start_index] = 255;
           }
        }
     }
  }
  cur_digits[digit] = 128;
}

void ULixie::special_pane(uint8_t index, bool enabled, CRGB col1, CRGB col2){
#ifdef PANE11
  if (reverse_string == 0) {
        uint16_t start_index = index;
        if (reverse_string == 1) {
           start_index = (n_digits - index) - 1;  
        }
	   special_panes_enabled[start_index] = enabled;

	   if(enabled){
		    if(col2.r != 0 || col2.g != 0 || col2.b != 0){ // use second color if defined
			     Serial.println("\n\n\nSPECIAL COL 2");
   	       special_panes_color[(start_index*2)+1] = col1;
		   	   special_panes_color[start_index*2]     = col2;
		    } else { // if not, just use col1 for both special pane LEDs instead
			     special_panes_color[(start_index*2)+1] = col1;
			     special_panes_color[start_index*2]     = col1;
		    }
	   } else {
		    special_panes_color[index*2+1] = CRGB(0,0,0);
		    special_panes_color[index*2]   = CRGB(0,0,0);
	   }
   }
#endif
}

void ULixie::color_all(uint8_t layer, CRGB col){
  for(uint16_t i = 0; i < n_LEDs; i++){
     if(layer == ON){
       col_on[i] = col;
     }
     else if(layer == OFF){
       col_off[i] = col;
     }
  }
  for (uint8_t i = 0; i < n_digits; i++) {
     if (layer == ON) {
        cur_col_on[i] = col;
     } else if (layer == OFF) {
        cur_col_off[i] = col;
     }
  }
}

void ULixie::color_all_dual(uint8_t layer, CRGB col_left, CRGB col_right){
  bool side = 1;
  for(uint16_t i = 0; i < n_LEDs; i++){
    if(i % (leds_per_digit/2) == 0){
      side = !side;
    }
    
    if(layer == ON){
      if(side){
        col_on[i] = col_left;
      }
      else{
        col_on[i] = col_right;
      }
    }
    else if(layer == OFF){
      if(side){
        col_off[i] = col_left;
      }
      else{
        col_off[i] = col_right;
      }
    }
  }
  for (uint8_t i = 0; i < n_digits; i++) {
     if (layer == ON) {
        cur_col_on[i] = col_left;
     } else if (layer == OFF) {
        cur_col_off[i] = col_left;
     }
  }
  // this updta eof the color registers is not correct ! 
  // but there is no alternative. Color register will be correct after
  // an absolute call.
}

CRGB ULixie::get_color_display(uint8_t display, uint8_t layer){
   if (layer == ON) {
      return cur_col_on[display];
   } else if (layer == OFF) {
      return cur_col_off[display];
   } else { 
      return CRGB(0,0,0);
   }  
}

void ULixie::color_display(uint8_t display, uint8_t layer, CRGB col){
  uint16_t start_index = display*leds_per_digit;  
  if (reverse_string == 1) {
     start_index = (n_digits - display - 1)*leds_per_digit;  
  }
  for(uint16_t i = 0; i < leds_per_digit; i++){
    if(layer == ON){
      col_on[start_index+i] = col;
    }
    else if(layer == OFF){
      col_off[start_index+i] = col;
    }
  }
  if (reverse_string == 1) {
    if (layer == ON){
      cur_col_on[n_digits - display - 1] = col;
    } else if (layer == OFF) {
      cur_col_off[n_digits - display - 1] = col;
    }
  } else {
    if (layer == ON) {
      cur_col_on[display] = col;
    } else if (layer == OFF) {
      cur_col_off[display] = col;
    }
  } 
}

void ULixie::gradient_rgb(uint8_t layer, CRGB col_left, CRGB col_right){
  for(uint16_t i = 0; i < n_LEDs; i++){
    float progress = 1-(led_to_x_pos(i)/float(max_x_pos));

    CRGB col_out = CRGB(0,0,0);
    col_out.r = (col_right.r*(1-progress)) + (col_left.r*(progress));
    col_out.g = (col_right.g*(1-progress)) + (col_left.g*(progress));
    col_out.b = (col_right.b*(1-progress)) + (col_left.b*(progress));
    
    if(layer == ON){
      col_on[i] = col_out;
    }
    else if(layer == OFF){
      col_off[i] = col_out;
    }
  }
  for (uint8_t i = 0; i < n_digits; i++) {
     float progress = 1-((i + 1) / float(n_digits + 1));
     CRGB col_out = CRGB(0,0,0);
     col_out.r = (col_right.r*(1-progress)) + (col_left.r*(progress));
     col_out.g = (col_right.g*(1-progress)) + (col_left.g*(progress));
     col_out.b = (col_right.b*(1-progress)) + (col_left.b*(progress));
     if (layer == ON) {
        cur_col_on[i] = col_out;
     } else if (layer == OFF) {
        cur_col_off[i] = col_out;
     }
  }
  // this update eof the color registers is not correct ! 
  // but there is no alternative. Color register will be correct after
  // an absolute call.
}

void ULixie::brightness(float level){
  //FastLED.setBrightness(255*level); // NOT SUPPORTED WITH CLEDCONTROLLER :(
  bright = level; // We instead enforce brightness in the animation ISR
}

void ULixie::brightness(double level){
  //FastLED.setBrightness(255*level); // NOT SUPPORTED WITH CLEDCONTROLLER :(
  bright = level; // We instead enforce brightness in the animation ISR
}

void ULixie::fade_in(){
  for(int16_t i = 0; i < 255; i++){
    brightness(i/255.0);
    FastLED.delay(1);
  }
  brightness(1.0);
}

void ULixie::fade_out(){
  for(int16_t i = 255; i > 0; i--){
    brightness(i/255.0);
    FastLED.delay(1);
  }
  brightness(0.0);
}

void ULixie::streak(CRGB col, float pos, uint8_t blur){
  float pos_whole = pos*n_digits*6; // 6 X-positions in a single display
  
  for(uint16_t i = 0; i < n_LEDs; i++){
    uint16_t pos_delta = abs(led_to_x_pos(i) - pos_whole);
    if(pos_delta > blur){
      pos_delta = blur;
    }
    float pos_level = 1-(pos_delta/float(blur));
    
    pos_level *= pos_level; // Squared for sharper falloff
    
    lix_leds[i] = CRGB(col.r * pos_level, col.g * pos_level, col.b * pos_level);
  }
  lix_controller->showLeds();
}

void ULixie::sweep_color(CRGB col, uint16_t speed, uint8_t blur, bool reverse){
  stop_animation();
  sweep_gradient(col, col, speed, blur, reverse);
  start_animation();
}

void ULixie::sweep_gradient(CRGB col_left, CRGB col_right, uint16_t speed, uint8_t blur, bool reverse){
  stop_animation();
  
  if(!reverse){
    for(int16_t sweep_pos = (blur*-1); sweep_pos <= max_x_pos+(blur); sweep_pos++){
      int16_t sweep_pos_fixed = sweep_pos;
      if(sweep_pos < 0){
        sweep_pos_fixed = 0;
      }
      if(sweep_pos > max_x_pos){
        sweep_pos_fixed = max_x_pos;
      }
      float progress = 1-(sweep_pos_fixed/float(max_x_pos));

      CRGB col_out = CRGB(0,0,0);
      col_out.r = (col_right.r*(1-progress)) + (col_left.r*(progress));
      col_out.g = (col_right.g*(1-progress)) + (col_left.g*(progress));
      col_out.b = (col_right.b*(1-progress)) + (col_left.b*(progress));
      
      streak(col_out, 1-progress, blur);
      FastLED.delay(speed);
    }
  }
  else{
    for(int16_t sweep_pos = max_x_pos+(blur); sweep_pos >= (blur*-1); sweep_pos--){
      int16_t sweep_pos_fixed = sweep_pos;
      if(sweep_pos < 0){
        sweep_pos_fixed = 0;
      }
      if(sweep_pos > max_x_pos){
        sweep_pos_fixed = max_x_pos;
      }
      float progress = 1-(sweep_pos_fixed/float(max_x_pos));

      CRGB col_out = CRGB(0,0,0);
      col_out.r = (col_right.r*(1-progress)) + (col_left.r*(progress));
      col_out.g = (col_right.g*(1-progress)) + (col_left.g*(progress));
      col_out.b = (col_right.b*(1-progress)) + (col_left.b*(progress));
      
      streak(col_out, progress, blur);
      FastLED.delay(speed);
    }
  }
  start_animation();
}

uint8_t ULixie::get_size(uint32_t input){
  uint8_t places = 1;
  while(input > 9){
    places++;
    input /= 10;
  }
  return places;
}

void ULixie::nixie(){
  color_all(ON, CRGB(255, 70, 7));
  color_all(OFF, CRGB(0, 3, 8));  
}

void ULixie::white_balance(CRGB c_adj){
  lix_controller->setTemperature(c_adj);
}

void ULixie::rainbow(uint8_t r_hue, uint8_t r_sep){
  for(uint8_t i = 0; i < n_digits; i++){
    color_display(i, ON, CHSV(r_hue,255,255));
    r_hue+=r_sep;
  }
}

void ULixie::clear(bool show_change){
  clear_all();
  if(show_change){
    mask_update();
  }
}

void ULixie::clear_digit(uint8_t index, bool show_change){
  clear_digit(index);
  if(show_change) {
    mask_update();
  }
}

void ULixie::show(){
  mask_update();
}

// BEGIN LIXIE 1 DEPRECATED FUNCTIONS

void ULixie::brightness(uint8_t b){
  brightness( b/255.0 ); // Forward to newer float function
}

void ULixie::write_flip(uint32_t input, uint16_t flip_time, uint8_t flip_speed){
  // This animation no longer supported, crossfade is used instead
  transition_type(CROSSFADE);
  transition_time(flip_time);
  write(input);
}

void ULixie::write_fade(uint32_t input, uint16_t fade_time){
  transition_type(CROSSFADE);
  transition_time(fade_time);
  write(input);
}

void ULixie::sweep(CRGB col, uint8_t speed){
  sweep_color(col, speed, 3, false);
}

void ULixie::progress(float percent, CRGB col1, CRGB col2){
  uint16_t crossover_whole = percent * n_digits;
  for(uint8_t i = 0; i < n_digits; i++){
    if(n_digits-i-1 > crossover_whole){
      color_display(n_digits-i-1, ON, col1);
      color_display(n_digits-i-1, OFF, col1);
    }
    else{
      color_display(n_digits-i-1, ON, col2);
      color_display(n_digits-i-1, OFF, col2);
    }
  }
}

void ULixie::fill_fade_in(CRGB col, uint8_t fade_speed){
  for(float fade = 0.0; fade < 1.0; fade += 0.05){
    for(uint16_t i = 0; i < n_LEDs; i++){
      lix_leds[i].r = col.r*fade;
      lix_leds[i].g = col.g*fade;
      lix_leds[i].b = col.b*fade;
    }
    
    FastLED.show();
    delay(fade_speed);
  }
}

void ULixie::fill_fade_out(CRGB col, uint8_t fade_speed){
  for(float fade = 1; fade > 0; fade -= 0.05){
    for(uint16_t i = 0; i < n_LEDs; i++){
      lix_leds[i].r = col.r*fade;
      lix_leds[i].g = col.g*fade;
      lix_leds[i].b = col.b*fade;
    }
    
    FastLED.show();
    delay(fade_speed);
  }
}

void ULixie::color(uint8_t r, uint8_t g, uint8_t b){
  color_all(ON,CRGB(r,g,b));
}
void ULixie::color(CRGB c){
  color_all(ON,c);
}
void ULixie::color(uint8_t r, uint8_t g, uint8_t b, uint8_t index){
  color_display(index, ON, CRGB(r,g,b));
}
void ULixie::color(CRGB c, uint8_t index){
  color_display(index, ON, c);
}
void ULixie::color_off(uint8_t r, uint8_t g, uint8_t b){
  color_all(OFF,CRGB(r,g,b));
}
void ULixie::color_off(CRGB c){
  color_all(OFF,c);
}
void ULixie::color_off(uint8_t r, uint8_t g, uint8_t b, uint8_t index){
  color_display(index, OFF, CRGB(r,g,b));
}
void ULixie::color_off(CRGB c, uint8_t index){
  color_display(index, OFF, c);
}

void ULixie::color_fade(CRGB col, uint16_t duration){
  // not supported
  color_all(ON,col);
}
void ULixie::color_fade(CRGB col, uint16_t duration, uint8_t index){
  // not supported
  color_display(index,ON,col);
}
void ULixie::color_array_fade(CRGB *cols, uint16_t duration){
  // support removed
}
void ULixie::color_array_fade(CHSV *cols, uint16_t duration){
  // support removed
}
void ULixie::color_wipe(CRGB col1, CRGB col2){
  gradient_rgb(ON,col1,col2);
}
void ULixie::nixie_mode(bool enabled, bool has_aura){
  // enabled removed
  // has_aura removed
  nixie();
}
void ULixie::nixie_aura_intensity(uint8_t val){
  // support removed
}
    
