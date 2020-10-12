/*
  ULixie_write.cpp - Library for controlling the DIAMEX lixie
  
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
//#include "ULixie_write.h"

//uint8_t ULixie::get_n_digits() {
//   return n_digits; 
//}

//uint8_t ULixie::get_trans_effect()  {
//   return trans_effect;
//}

//uint32_t ULixie::get_trans_time()  {
//  return trans_time;
//}

void ULixie::transition_type(uint8_t type){
   trans_type = type;
} 

void ULixie::transition_time(uint16_t ms){
   trans_time = ms;
}

void ULixie::transition_effect(uint16_t type){
   trans_effect = type;
}

//uint8_t ULixie::get_size(uint32_t input){
//   uint8_t places = 1;
//   while(input > 9){
//        places++;
//        input /= 10;
//   }
//   return places;
//}

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

void ULixie::integer_to_digits(char *outbuf, int32_t input) {
/*
 *  can handle positive numbers and negative numbers.
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
        float_to_digits(next_digits,(float) input, 1);
        return;
    } else if (input > 999999) {
        float_to_digits(next_digits,(float) input, 1);
        return;
    } else if (absinput == 0) {
        next_digits = "     0";
    } else {
       n_place = 100000;
       for (i = 0; i < n_digits; i++) {
            num = (char)(absinput / n_place) + 48;
            if ((z == i) && (num == 48)) {
                num = ' ';
                z++;
            } else if (input < 0) {
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
    }
    for (i = 0; i < n_digits; i++) {
        outbuf[i] = next_digits[i];
    }
    free(next_digits);
}

void ULixie::floatold_to_digits(char * outbuf,float input_raw, uint8_t dec_places) {
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
        if (char_is_number(buf[i]) == true) {
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
    for (i = 0; i < n_digits; i++) {
        outbuf[i] = next_digits[i];
    }
    free(next_digits);
    free(buf);
}   

void ULixie::float_to_digits(char *buf, float input_raw, uint8_t dec_places) {
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
   int16_t digitspushed = 0;
   uint8_t comma = 0, i = 0, exponent = 0;
  
   next_digits = new char[n_digits+1];
//  char * buf = new char[20];

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
            next_digits[j] = '9' - 10;
            if ((j == 2) || (j == 3)) {
                next_digits[j] = '9';;
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
               break;          
// do not push anything if comma would be in last postion
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
           if (char_is_number(buf[i]) == true) {
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
       buf[i] = next_digits[i];
       if (j+1 < exponent) {
           if (char_is_number(next_digits[j]) == true) buf[j] = next_digits[j] + 10;
       } else if ((j+1) > (digitspushed - comma)) {
// do nothing -- buf[j] = next_digits[j];             
       } else {
           if (char_is_number(next_digits[j]) == true) buf[j] = next_digits[j] - 10;
       }
   }
//  free(buf);
   free(next_digits);
}   

void ULixie::write(uint32_t input) {
    char * next_digits;
    next_digits = new char[n_digits];
    integer_to_digits(next_digits, (int32_t) input);
    write(next_digits);
    free(next_digits);
}

void ULixie::write(int32_t input){
    char * next_digits;
    next_digits = new char[n_digits];
    integer_to_digits(next_digits, input);
    write(next_digits);
    free(next_digits);
}

void ULixie::write_float(float input_raw, uint8_t dec_places) {
    char * next_digits;
    char num;
    CRGB col;
    next_digits = new char[n_digits];
    float_to_digits(next_digits, input_raw, dec_places);
    for (uint8_t j = 0; j < n_digits; j++) {
        col = get_color_display(j,ON);
        num = next_digits[j];
        if ((char_is_number(next_digits[j]) == true)) {
            color_display(j,ON, col);
            next_digits[j] = num;
        } else if ((char_is_number(next_digits[j]-10) == true)) {
            color_display(j,ON,CRGB(col.b/4,col.g/4,col.r/4));
            next_digits[j] = num + 10;
        } else if ((char_is_number(next_digits[j]+10) == true)) {
            color_display(j,ON,col);
            next_digits[j] = num - 10;
        } else {
            color_display(j,ON,col);
            next_digits[j] = num;
        }
   }  
   write(next_digits);
   free(next_digits);
}

void ULixie::write_floatplus(float input_raw, CRGB col) {
    char * next_digits;
    char num;
    next_digits = new char[n_digits];
    float_to_digits(next_digits, input_raw, 5);
    for (uint8_t j = 0; j < n_digits; j++) {
        num = next_digits[j];
        if ((char_is_number(next_digits[j]) == true)) {
            color_display(j,ON, col);
            next_digits[j] = num;
        } else  if ((char_is_number(next_digits[j]-10) == true)) {
            color_display(j,ON,CRGB(col.b/4,col.g/4,col.r/4));
            next_digits[j] = num + 10;
        } else if ((char_is_number(next_digits[j]+10) == true)) {
            color_display(j,ON,col);
            next_digits[j] = num - 10;
        } else {
            color_display(j,ON,col);
            next_digits[j] = num;
        } 
    }  
    write(next_digits);
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
   
    } else { 
    
// reverse_string = 0.....

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

// 
    } 
// endif for if (reverse_string == 0)
}

void ULixie::write(String input) {
    switch(trans_effect) {
    case INSTANT: write_instant(input); break;
    case BLINKER: write_blinker(input); break;
    case SCROLLLEFT: write_scroll(input, LEFTRIGHT); break;
    case SCROLLFLAPLEFT: write_scrollflap(input,LEFTRIGHT); break;
    case ONEBYONELEFT: write_onebyone(input, LEFTRIGHT); break;
    case SCROLLRIGHT: write_scroll(input, RIGHTLEFT); break;
    case SCROLLFLAPRIGHT: write_scrollflap(input,RIGHTLEFT); break;
    case ONEBYONERIGHT: write_onebyone(input, RIGHTLEFT); break;
    case GASPUMP: write_gaspump(input); break;
    case PINBALL: write_pinball(input); break;
    case SLOTMACHINE: write_slotmachine(input); break;
    case SLOTIIMACHINE: write_slotiimachine(input); break;
    default: 
    write_instant(input); 
    break; 
    }
}
