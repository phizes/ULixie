/* 
 *  
 * ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
/* ONEBYONE effect: The new value will be 'dropped' in from right to left, 
   thereby overwriting the previous indicated values. 
   - dir parameter indicates direction (left to right / right to left)
   - trans_time will indicate the dropping speed.
*/     

#ifndef ULixie_h
#include "ULixie.h"
#endif

void ULixie::write_onebyone(String input, uint8_t dir){

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

    uint8_t n_digits = get_n_digits();
    uint32_t trans_time = get_trans_time();
    
    prev_digits = new uint8_t[n_digits];
    next_digits = new uint8_t[n_digits];
    delay_times = new uint32_t[n_digits];
    delay_index = new uint8_t[n_digits];

    input_to_digits(prev_digits, next_digits, input);

    t_start = millis();
    t_now = millis();

    i = 0;
    while (i < n_digits) {
       if ((t_now - delaytime) > trans_time) {
          if (dir = LEFTRIGHT) {
             write_digit(n_digits - 1 - i, next_digits[n_digits - 1 - i]);
          } else {
             write_digit(i, next_digits[i]);
          }
          delaytime = t_now;
          mask_toggle();
          i++;
       }
       if (t_now % 20 == 0) {
          run();
          yield();
       }
       t_now = millis();         
    }      
    free(prev_digits);
    free(next_digits);
    free(delay_times);
    free(delay_index);
}
