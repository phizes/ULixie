/* 
 *  ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
/* BLINKER effect: after the intended value is written just like that 
   (like 'INSTANT'), the display will blink 10 times. This blinking part 
   of the write action.
   - trans_time will indicate the blinking speed.
*/

//#ifndef ULixie_h
#include "ULixie.h"
//#endif

void ULixie::write_blinker(String input){

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
