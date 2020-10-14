/* 
 * ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
/* GASPUMP effect: Like an old gaspump with electromechanical 'flappy' 
   7segnment displays. At start, all lixies that must display a digit 
   will spin in unison (all numbers equal) After one full cycle, every 
   next cycle the next display will stop at its intended value while 
   the rest keeps spinning.
   - trans_time will indicate the spinning speed.
*/

#ifndef ULixie_h
#include "ULixie.h"
#endif

void ULixie::write_gaspump(String input){

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
                      if ((next_digits[i] == num) || 
                          (next_digits[i] > 10)) {
                         delay_index[i] = 20;
                         reelsstopped++;
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
