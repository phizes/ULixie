/* 
 * ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
/* SLOTMACHINE effect: Every lixie display is a reel on a slot machine. 
   They spin and will stop on the number indicated in the call to this 
   write function. The reel slows down like a reel slot machine, next 
   reel will slow down once previous reel stopped. 
   - trans_time will indicate the spinning speed.
*/

#include "ULixie.h"

void ULixie::write_slotmachine(String input){

//    uint32_t delay_table[20] = {10,10,10,10,10,10, 10, 10, 10, 10,
//                                14,20,28,40,56,80,112,160,224,400};
    uint32_t delay_table[20] = {10,10,10,10,10,10, 10, 10, 12, 15,
                                18,22,27,33,39,47, 56, 68, 82,100};

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
          delay_index[i] = 10;
          delay_times[i] = t_now;
       } else {
          delay_index[i] = 19;
          delay_times[i] = 0;
          write_digit(i,128);
       }
    }
    reelsstopped = 0;
    while (reelsstopped < n_digits) {
       t_now = millis();
       if ((t_now % 20) == 0) {
          for (i = 0; i < n_digits; i++) {
             digitwritten = 0;
             if (delay_index[i] < 20) {
                for (j = i; j < n_digits; j++) {
                   if ((t_now - delay_times[j]) > 
                        (get_trans_time() * delay_table[delay_index[j]] / 10)) {
                      if (j == i) {
                         delay_index[j] += 1;
                      }
                      num = (read_digit(j) + random(9)) % 10;
                      if (next_digits[j] > 10) num = 128;
                      write_digit(j,num);
                      digitwritten++;
                      delay_times[j] = t_now;
                   }
                }
             }
             if ((delay_index[i] > 19) && (delay_times[i] > 0)) {
                reelsstopped = i + 1;
                delay_times[i] = 0;
                num = next_digits[i];
                write_digit(i,num);
                digitwritten++;
             }
             if (digitwritten > 0) mask_toggle();
          }      
          run();
          yield();
       } 
    }     
    free(prev_digits);
    free(next_digits);
    free(delay_times);
    free(delay_index);
}
