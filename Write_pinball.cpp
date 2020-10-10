/* 
 * ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
/* PINBALL effect: Like an old pinballmachine with electromechanical 
   score displays. In the first round, the previous value of the lixie 
   is taken into acount. They start spining to zero when the counter is 
   equal to their value (like the score reset on the pinball machine). 
   Then lixies that do not need to display a number are cleared an the 
   rest will take 2 complete 0-9 spin rounds in unison. In the third 
   round the lixies will stuck on their intended value. 
   - trans_time will indicate the spinning speed.
*/
#include "ULixie.h"

void ULixie::write_pinball(String input){

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
          delay_index[i] = 1;
          delay_times[i] = 0;
       }
    }
    reelsstopped = 0;
    num = 0;
    t_now = millis() + get_trans_time() + 1;
    while (reelsstopped < n_digits) {
       if ((t_now - delay_times[reelsstopped]) > get_trans_time()) {
          digitwritten = 0;
          for (i = 0; i < n_digits; i++) {
             if (delay_index[i] < 30) {
                delay_times[i] = t_now;
                delay_index[i] = delay_index[i] + 1;
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
                   }
                }
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
