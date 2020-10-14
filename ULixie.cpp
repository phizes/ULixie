/*
  ULixie.cpp - Library for controlling the DIAMEX lixie
  
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

//uint8_t get_size(uint32_t input);

// FastLED info for the LEDs
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define LEDS_PER_DIGIT 22
// #define PANE11

// namespace ULixie_global

uint8_t led_assignments_LixieII[LEDS_PER_DIGIT] = 
    { 1, 9, 4, 6, 255, 7, 3, 0, 2, 8, 5, 5, 8, 2, 0, 3, 7, 255, 6, 4, 9, 1  }; // 255 is extra pane
uint8_t x_offsets_LixieII[LEDS_PER_DIGIT] = 
    { 0, 0, 0, 0, 1,   1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4,   5, 5, 5, 5  };

uint8_t led_assignments_diamex[LEDS_PER_DIGIT] = 
    { 1, 3, 5, 7, 9, 0, 2, 4, 6, 8 };
uint8_t x_offsets_diamex[LEDS_PER_DIGIT] = 
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  };

CLEDController *lix_controller; // FastLED 

uint8_t n_digits;           // number of Lixies in the string
uint16_t n_LEDs;            // total number of LEDs in the string
uint8_t leds_per_digit = 22;// number of LEDs per Lixie in the string
uint8_t reverse_string = 0; // 0: Most Significat Lixie at end of string
                            // 1: Most Significat Lixie at start of string
uint16_t max_x_pos = 0;     // 

uint8_t *led_assignments;   // Order of panes in a Lixie
uint8_t *x_offsets;
CRGB *lix_leds;             // LED setting passed to FastLib library
CRGB *col_on;               // Current led ON (foreground) color
CRGB *col_off;              // Current led OFF (background) color

uint8_t *cur_digits;        // Digits being displayed currently
CRGB *cur_col_on;           // Current digit ON (foreground) color
CRGB *cur_col_off;          // Current digit OFF (background) color
        
uint8_t current_mask = 0;   // do we fade from 0 to 1 or 1 to 0
uint8_t *led_mask_0;        // used in fading (mask=0 -> fade from)
uint8_t *led_mask_1;        // used in fading (mask=0 -> fade to)
        
uint8_t trans_type = CROSSFADE;// current transition type between two
                            // different displayed values on single
                            // lixie
uint8_t trans_effect = INSTANT;// current transition effect between two
                            // different values on the whole string
uint32_t trans_time = 250;  // time to execute trans_type or trans_effect

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
Ticker lixie_animation;
#endif

bool *special_panes_enabled;
CRGB *special_panes_color;

//float mask_push = 1.0;
uint32_t t_lastanimate = 0;
uint32_t t_lastmaskupdate = 0;
bool mask_fade_finished = true;
bool mustnowanimate = false;
bool transition_mid_point = true;
float bright = 1.0;
bool background_updates = true;

// Wrapper funtions

uint8_t ULixie::get_n_digits() {
    return n_digits; 
}

uint32_t ULixie::get_trans_time()  {
    return trans_time;
}

uint8_t ULixie::get_trans_effect()  {
    return trans_effect;
}

uint16_t ULixie::led_to_x_pos(uint16_t led){
    uint16_t led_digit_pos = x_offsets[led%leds_per_digit];
  
    uint8_t complete_digits = 0;
    while(led >= leds_per_digit){
        led -= leds_per_digit;
        complete_digits += 1;
    }
  
    return max_x_pos - (led_digit_pos + (complete_digits*6));
}

void animate() {  
//  using namespace ULixie;
    uint32 t_animate = millis();
    float mask_fader = 1.0;
  
    if (((t_animate - t_lastanimate) < 20) && (mustnowanimate == false)) { 
     // 20ms is 50 frames per second, do nothing if this call is too soon
        return;
    }
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
    lix_leds[i] = new_col;  

#ifdef PANE11	
    // Check for special pane enabled for the current digit, and use its color instead if it is.
    uint8_t pcb_index = 0;
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
// Initialize all protected global variables
    leds_per_digit = 22;
    reverse_string = 0;
    n_LEDs = number_of_digits * leds_per_digit;
    n_digits = number_of_digits;
    max_x_pos = (number_of_digits * 6)-1;
  
    led_assignments = new uint8_t[LEDS_PER_DIGIT];
    x_offsets = new uint8_t[LEDS_PER_DIGIT];
  
    lix_leds = new CRGB[n_LEDs];  
    col_on = new CRGB[n_LEDs];
    col_off = new CRGB[n_LEDs];
  
    cur_digits = new uint8_t[n_digits];
    cur_col_on = new CRGB[n_digits];
    cur_col_off= new CRGB[n_digits];
  
    led_mask_0 = new uint8_t[n_LEDs];
    led_mask_1 = new uint8_t[n_LEDs];
    current_mask = 0;  

    trans_type = CROSSFADE;
    trans_time = 250;
    trans_effect = INSTANT;
// end of initialization of all protected global variables
  
#ifdef PANE11
    special_panes_enabled = new bool[n_digits];
    special_panes_color = new CRGB[n_digits*2];
    for(uint16_t i = 0; i < n_digits*2; i++){
        special_panes_color[i] = CRGB(0,0,0);
    }
#endif
  
    for (uint16_t i = 0; i< leds_per_digit; i++) {
        led_assignments[i] = led_assignments_LixieII[i];
        x_offsets[i] = x_offsets_LixieII[i];
    }

    for(uint16_t i = 0; i < n_LEDs; i++){
        led_mask_0[i] = 0;
        led_mask_1[i] = 0;
    
        col_on[i] = CRGB(222,222,222);
        col_off[i] = CRGB(2,6,0);
    }
    for(uint16_t i = 0; i < n_digits; i++){
        cur_digits[i] = 0;
        cur_col_on[i] = CRGB(222,222,222);
        cur_col_off[i] = CRGB(2,6,0);
    }
  
    build_controller(pin);
}

void ULixie::setDiamex(uint8_t number_of_digits) {
    leds_per_digit = 10;
    reverse_string = 1;
    n_LEDs = number_of_digits * leds_per_digit;
    n_digits = number_of_digits;
    max_x_pos = (number_of_digits * 6)-1;

    for (uint16_t i = 0; i< leds_per_digit; i++) {
        led_assignments[i] = led_assignments_diamex[i];
        x_offsets[i] = 0;
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

uint8_t ULixie::read_digit(uint8_t num) {
    if (cur_digits[num] < 10) {
        return cur_digits[num];
    } else if (cur_digits[num] == 128) {
        return 128;
    } else {  // digit is not number or space thus special pane
        return 255;
    }
}

void ULixie::write_digit(uint8_t digit, uint8_t num){
    uint16_t start_index = leds_per_digit*digit;
    if (!(digit < n_digits)) {
        Serial.println("Digit more than lixies");
        return;
    }
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
        } else if(layer == OFF){
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
           } else{
                col_on[i] = col_right;
           }
        } else if(layer == OFF){
           if(side){
                col_off[i] = col_left;
           } else{
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
        } else if(layer == OFF){
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
        } else if(layer == OFF){
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
    } else{
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

//uint8_t ULixie::get_size(uint32_t input){
//  uint8_t places = 1;
//  while(input > 9){
//    places++;
//    input /= 10;
//  }
//  return places;
//}

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

// BEGIN DIGIT WRITE FUNCTIONS

bool ULixie::char_is_number(char input){
    if(input <= 57 && input >= 48) // if equal to or between ASCII '0' and '9'
        return true;
    else
        return false;
}

void ULixie::input_to_digits(uint8_t * pdigits, uint8_t * ndigits, String input) {
    uint8_t i;
    for (i = 0; i < n_digits; i++) {
        if (i < input.length()) {
            char c = input.charAt(i);
            if (char_is_number(c)) {
                 ndigits[n_digits - i - 1] = c - '0';
            } else if (c == ' ') {
                 ndigits[n_digits - i - 1] = 128;
            } else {
                 ndigits[n_digits - i - 1] = 255;
            }
        } else {
            ndigits[n_digits - i - 1] = 128; //pad with spaces
        }
        if (cur_digits[i] < 10) {
            pdigits[i] = cur_digits[i];
        } else if (cur_digits[i] == 128) {
            pdigits[i] = 128;
        } else {  // digit is not number or space thus special pane
            pdigits[i] = 255;
        }  
    }      
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
            } else {
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
            } else {
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
    
} // endif for if (reverse_string == 0)
}
