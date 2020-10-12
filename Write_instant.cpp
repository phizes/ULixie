/* 
 *  ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
/*  INSTANT effect: The intended value is written to the display in one go 
    without any other effects. This effect is used a default when the 
    trans_effect variable contains a non described effect number. 
*/

#ifndef ULixie_h
#include "ULixie.h"
#endif

void ULixie::write_instant(String input){

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

    input_to_digits(prev_digits, next_digits, input);
    t_start = millis();
    t_now = millis();
//     clear_all(); --> colors must be set befor call to write
    for (i = 0; i < n_digits; i++) {
       write_digit(i, next_digits[i]);
    }
    mask_toggle();
    yield();
 
    free(prev_digits);
    free(next_digits);
    free(delay_times);
    free(delay_index);
}
