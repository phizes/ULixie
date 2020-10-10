/* 
 * ULixie write funtion writes sting to the lixie displays. 
   It takes into account the settings of the trans_type, trans_time, and
   trans_effect variables. Trans_effects may take a (very) long time,
   No problem, during the trans_effect yield() is called every 20 ms.
*/
/* SLOTIIMACHINE effect: A variant on the slotmachine theme. 
   Works like SLOTMACHINE, except next reel will slow down 
   before preivious reel stops. 
   - trans time indicates the spinning speed
*/

#include "ULixie.h"

void ULixie::write_slotiimachine(String input){

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
          delay_index[i] = 2 * i;
          if (delay_index[i] > 16) delay_index[i] = 16;
          delay_times[i] = t_now - i;
       } else {
          delay_index[i] = 19;
          delay_times[i] = 0;
          write_digit(i,128);
       }
    }
    reelsstopped = 0;
    while (reelsstopped < n_digits) {
       t_now = millis();
       if (t_now % 20 == 0) {         
          digitwritten = 0;
          for (i = 0; i < n_digits; i++) {
             if (delay_index[i] < 20) {
                if ((t_now - delay_times[i]) > 
                     (get_trans_time() * delay_table[delay_index[i]]) / 10) {
                   num = read_digit(i) + random(9) % 10;
                   if (next_digits[i] > 10) num = 128;
                   delay_index[i] = delay_index[i] + 1;
                   delay_times[i] = t_now;
                   if (delay_index[i] > 19) {  // table has 20 entries
                      reelsstopped++;
                      num = next_digits[i];
                   }
                   write_digit(i,num);
                   digitwritten++;
                }
             }
          }
          if (digitwritten > 0) mask_toggle();
//             run();      // --> mask_toggle();
          yield();
       }
    }
    free(prev_digits);
    free(next_digits);
    free(delay_times);
    free(delay_index);
}
