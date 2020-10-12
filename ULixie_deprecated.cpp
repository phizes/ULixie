/*
  ULixie_deprecated.cpp - Library for controlling the DIAMEX lixie
  
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
//  uint8_t n_digits = get_n_digits();
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
