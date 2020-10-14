/*
  ULixie_write.cpp - Library for controlling Lixie-IIs and their cousins
  
  Created by Phons Bloemen (sep 30 2020)
  Tested for use with DIAMEX lixies.
  
  Based on Lixie_II library by Connor Nishijma July 6th, 2019
  Is partially upward compatible with it

  Released under the GPLv3 License
*/

#include "ULixie.h"

void ULixie::integer_to_digits(char *outbuf, int32_t input) {
/**
 *  Converts an integer to an output string displayable on lixies
 *  can handle positive numbers and negative numbers.
 *  Use for all Lixies also without an 11th pane.
 *  If a negative number is passed, the result will 
 *  be prepended with an extra zero to denote the minus sign.
 *  if the value is 'out of bounds' the value is passed to the floatplus()
 *  function (which can handle bigger absolute values).
 *  If (left side) padding is needed, it will be with spaces.
 *  \param outbuf  Pointer to char array for output string
 *  \param input   The 32-bit integer to convert to string
 */
    char * next_digits;
    uint8_t i, z;
    int32_t n_place, absinput;
    char num; 
    uint8_t n_digits = get_n_digits();
    
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
/**
 *  Converts a float to an output string displayable on lixies
 *  \param outbuf      Pointer to char array for output string
 *  \param input_raw   The float to convert to string
 *  \param dec_places  Decimal places to take into account
 */
    char * next_digits;
    uint8_t comma = 0, i = 0;
    uint8_t n_digits = get_n_digits();

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
/**
 * Converts a float to an output string displayable on lixies
 * Note: The output string will contain number chars (48-57), 
 * number+14 chars (62-71) for 'dimming' and number-14 chars (34-43)
 * for blinking. it also contains char 32 for spaces and 46 (.) for 
 * a special pane if it exists. ass the output string AS IT COMES OUT 
 * on to the ULixie::write funtion toi get these effecs. 
 *  \param outbuf      Pointer to char array for output string
 *  \param input_raw   The float to convert to string
 *  \param dec_places  Decimal places to take into account
 */
    char * next_digits;
    int16_t digitspushed = 0;
    uint8_t comma = 0, i = 0, exponent = 0;
    uint8_t n_digits = get_n_digits();
  
    next_digits = new char[n_digits+1];

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
            if (char_is_number(next_digits[j]) == true) buf[j] = next_digits[j] + 14;
        } else if ((j+1) > (digitspushed - comma)) {
// do nothing -- buf[j] = next_digits[j];             
        } else {
            if (char_is_number(next_digits[j]) == true) buf[j] = next_digits[j] - 14;
        }
    }
//  free(buf);
    free(next_digits);
}   

void ULixie::write(uint32_t input) {
/**
 *  Displays an integer on the lixies
 *  can handle positive numbers and negative numbers.
 *  Internally uses the ULixie::inter_to_digits function
 *  \param input   The 32-bitsigned  integer to display
 */
    char * next_digits;
    uint8_t n_digits = get_n_digits();
    next_digits = new char[n_digits];
    integer_to_digits(next_digits, (int32_t) input);
    write(next_digits);
    free(next_digits);
}

void ULixie::write(int32_t input){
/**
 *  Displays an integer on the lixies
 *  can handle positive numbers and negative numbers.
 *  Internally uses the ULixie::inter_to_digits function
 *  \param input   The 32-bit integer to display
 */
    char * next_digits;
    uint8_t n_digits = get_n_digits();
    next_digits = new char[n_digits];
    integer_to_digits(next_digits, input);
    write(next_digits);
    free(next_digits);
}

void ULixie::write_float(float input_raw, uint8_t dec_places) {
/**
 * Writes a float directly onto the lixies. Uses 
 * the float_to_digits function, and processes the 
 * number chars (48-57), number+14 chars (62-71) for 'dimming' 
 * and number-14 chars (34-43) for blinking. The 'cleaned' string
 * (containing only numbers 48-57, space (32) and . (46) is passed to 
 * ULixie::write function. 
 *  \param outbuf      Pointer to char array for output string
 *  \param input_raw   The float to convert to string
 *  \param dec_places  Decimal places to take into account
 */
    char * next_digits;
    char num;
    CRGB col;
    uint8_t n_digits = get_n_digits();
    next_digits = new char[n_digits];
    float_to_digits(next_digits, input_raw, dec_places);
    for (uint8_t j = 0; j < n_digits; j++) {
        col = get_color_display(j,ON);
        num = next_digits[j];
        if ((char_is_number(next_digits[j]) == true)) {
            color_display(j,ON, col);
            next_digits[j] = num;
        } else if ((char_is_number(next_digits[j]-14) == true)) {
            color_display(j,ON,CRGB(col.b/4,col.g/4,col.r/4));
            next_digits[j] = num + 10;
        } else if ((char_is_number(next_digits[j]+14) == true)) {
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
    uint8_t n_digits = get_n_digits();
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

void ULixie::write(String input) {
/**
 * Writes a string directly on the lixies, 
 * takes into account current settings of trans_time, trans_effect and
 * trans_type.   Char 32 in the string is used for an 'empty' 
 * lixie (which shows the current backgroud color). A char '.' (46) 
 * is used for an 11th pane if it exists. 
 *  \param input   The string to write
 */

    switch(get_trans_effect()) {
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
