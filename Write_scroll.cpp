/* 
 *  ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
/* SCROLL effect: The new value will be 'pushed' in from the left side, 
   thereby 'pushing' forward the previous indicated values.
   - dir will indicate the direction (left to right / right to left) 
   - trans_time will indicate the pushing speed.
*/        

#ifndef ULixie_h
#include "ULixie.h"
#endif

void ULixie::write_scroll(String input, uint8_t dir){

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
                if (dir == LEFTRIGHT) {
                   write_digit(i,prev_digits[j]);
                } else {
                   write_digit((n_digits -1) -i, prev_digits[(n_digits-1)-j]);
                }
             } else {
                j = j - n_digits;
                if (dir == LEFTRIGHT) {
                   write_digit(i,next_digits[j]);
                } else {
                   write_digit((n_digits -1) -i, next_digits[(n_digits-1)-j]);
                }
             } 
             digitwritten++;
             delaytime = t_now;
          }
          if (digitwritten > 0) mask_toggle();
          reelsstopped++;
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
