/* 
  Ulixie.h - Library for controlling the Lixie II and tist cousins!
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

#ifndef ULixie_h
#define ULixie_h

#include "Arduino.h"

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	#include <Ticker.h>	// ESP ONLY
	// FastLED has issues with the ESP8266, especially
	// when used with networking, so we fix that here.
	#define FASTLED_ESP8266_RAW_PIN_ORDER
	#define FASTLED_ALLOW_INTERRUPTS 		0
	#define FASTLED_INTERRUPT_RETRY_COUNT	0
#endif

// Aside from those issues, it's my tool of choice for WS2812B
#include "FastLED.h"

#define ON  1
#define OFF 0
#define LEFTRIGHT 0
#define RIGHTLEFT 1

#define INSTANT   		          0
#define CROSSFADE 		          1
#define CROSSFADE2              2
#define DEEPFADE                3
#define ONEBYONELEFT            4
#define ONEBYONERIGHT           5
#define SCROLLLEFT              6
#define SCROLLRIGHT             7
#define SCROLLFLAPLEFT          8
#define SCROLLFLAPRIGHT         9
#define BLINKER                 10
#define SLOTMACHINE             11
#define PINBALL                 12
#define GASPUMP                 13
#define SLOTIIMACHINE		        14

// Functions
class ULixie
{
	public:
		ULixie(const uint8_t pin, uint8_t n_digits);
		void setDiamex(uint8_t n_digits);
		void build_controller(const uint8_t pin);
		void begin();
		void transition_type(uint8_t type);
		void transition_time(uint16_t ms);
		void max_power(uint8_t V, uint16_t mA);
		void color_all(uint8_t layer, CRGB col);
		void color_all_dual(uint8_t layer, CRGB col_left, CRGB col_right);
		void color_display(uint8_t display, uint8_t layer, CRGB col);
		void gradient_rgb(uint8_t layer, CRGB col_left, CRGB col_right);
		void start_animation();
		void stop_animation();
		void write(String input);
		void write(uint32_t input);
		void write_float(float input, uint8_t dec_places = 1);
		void clear_all();
		void write_digit(uint8_t digit, uint8_t num);
		void push_digit(uint8_t number);
		void clear_digit(uint8_t digit, uint8_t num);
		void special_pane(uint8_t index, bool enabled, CRGB col1 = CRGB(0,0,0), CRGB col2 = CRGB(0,0,0));
		void mask_update();
		void fade_in();
		void fade_out();
		void brightness(float level);
  	    void brightness(double level);
		void run();
		void wait();
		void streak(CRGB col, float pos, uint8_t blur);
		void sweep_color(CRGB col, uint16_t speed, uint8_t blur, bool reverse = false);
		void sweep_gradient(CRGB col_left, CRGB col_right, uint16_t speed, uint8_t blur, bool reverse = false);
		void nixie();
		void white_balance(CRGB c_adj);
		void rainbow(uint8_t r_hue, uint8_t r_sep);
    //.----------------------------------------------
    // Lixie_II with changed / updated code
    // ----------------------------------------------

    //.----------------------------------------------
    // New specific ULixie funtions
    // ----------------------------------------------

        void write(int32_t input); 
        void transition_effect(uint16_t type);
        CRGB get_color_display(uint8_t display, uint8_t layer);
        CRGB * readColLayer(uint8_t layer);
        uint8_t * readIntValues();
        String readString();   
        uint8_t read_digit(uint8_t num);
        void write_floatplus(float input, CRGB col);
        void mask_toggle();
        void integer_to_digits(char * outbuf, int32_t input);
        void floatold_to_digits(char * outbuf, float input, uint8 n_dec);
        void float_to_digits(char *outbuf, float input, uint8_t n_dec);
                
		// ----------------------------------------------
		// Deprecated Lixie 1 functions and overloads:
		// ----------------------------------------------
		
		void brightness(uint8_t b);
		void clear(bool show_change = true);
		void clear_digit(uint8_t index, bool show_change = true);
		void show();
		void write_flip(uint32_t input, uint16_t flip_time = 100, uint8_t flip_speed = 10);
		void write_fade(uint32_t input, uint16_t fade_time = 250);
		void sweep(CRGB col, uint8_t speed = 15);
		void progress(float percent, CRGB col1, CRGB col2);
		void fill_fade_in(CRGB col, uint8_t fade_speed = 20);
		void fill_fade_out(CRGB col, uint8_t fade_speed = 20);
		void color(uint8_t r, uint8_t g, uint8_t b);
		void color(CRGB c);
		void color(uint8_t r, uint8_t g, uint8_t b, uint8_t index);
		void color(CRGB c, uint8_t index);
		void color_off(uint8_t r, uint8_t g, uint8_t b);
		void color_off(CRGB c);
		void color_off(uint8_t r, uint8_t g, uint8_t b, uint8_t index);
		void color_off(CRGB c, uint8_t index);
		void color_fade(CRGB col, uint16_t duration);
		void color_fade(CRGB col, uint16_t duration, uint8_t index);
		void color_array_fade(CRGB *cols, uint16_t duration);
		void color_array_fade(CHSV *cols, uint16_t duration);
		void color_wipe(CRGB col1, CRGB col2);
		void nixie_mode(bool enabled, bool has_aura = true);
		void nixie_aura_intensity(uint8_t val);
		
	private:
        uint8_t n_digits;           // number of Lixies in the string
        uint16_t n_LEDs;            // total number of LEDs in the string
        uint8_t leds_per_digit;     // number of LEDs per Lixie in the string
        uint8_t reverse_string;     // 0: Most Significat Lixie at end of string
                                    // 1: Most Significat Lixie at start of string
        uint8_t *led_assignments;   // Order of panes in a Lixie
        
        uint8_t *cur_digits;        // Digits being displayed currently
        CRGB *lix_leds;             // LED setting passed to FastLib library
        CRGB *cur_col_on;           // Current ON (foreground) color
        CRGB *cur_col_off;          // Current OFF (background) color
        
        uint8_t current_mask;       // do we fade from 0 to 1 or 1 to 0
        uint8_t *led_mask_0;        // used in fading (mask=0 -> fade from)
        uint8_t *led_mask_1;        // used in fading (mask=0 -> fade to)
        
        uint8_t trans_type;         // current transition type between two
                                    // different displayed values on single
                                    // lixie
        uint8_t trans_effect;       // current transition effect between two
                                    // different values on the whole string
        uint32_t trans_time;        // time to execute trans_type or trans_effect

//		uint8_t get_size(uint32_t input);
//		uint8_t get_n_digits();
//		uint8_t get_trans_effect();
//		uint32_t get_trans_time();
		bool char_is_number(char input);
		void input_to_digits(uint8_t * pdigits, uint8_t *ndigits, String input);

        void write_instant(String input);
        void write_blinker(String input);
        void write_gaspump(String input);
        void write_pinball(String input);
        void write_slotmachine(String input);
        void write_slotiimachine(String input);
        void write_onebyone(String input, uint8_t dir);
        void write_scroll(String input, uint8_t dir);
        void write_scrollflap(String input, uint8_t dir);
};    

#endif /* ULixie_H */
