/* ULixie.h - Universal Library for controlling Lixies and variants.
  Created by Phons Bloemen (sep 30 2020)
  
  Based on Lixie_II library by Connor Nishijma July 6th, 2019
  Is partially upward compatible with it
  Released under the GPLv3 License

Different types of Lixies i found on the internet.:

Lixie-I: (untested, data taken from Lixie-I library)
10 panes, 2 leds per digit, 20 leds per digit in the string.
Diigit number 0 on the right of the string.
[ 1, 9, 4, 6, 7, 3, 0, 2, 8, 5, 5, 8, 2, 0, 3, 7, 6, 4, 9, 1 ]

Lixie_II: (untested, data taken from lixie-II library)
11 panes, 2 lds per digit, 22 leds per digit in the string.
Digit number 0 on the right of the string.
[ 1, 9, 4, 6, 255, 7, 3, 0, 2, 8, 5, 5, 8, 2, 0, 3, 7, 255, 6, 4, 9, 1 ]

DIAMEX lixie: (tested)
10 panes, 2 leds per digit (paralel), 10 leds per digit in the string
Digit number 0 on the left of the string.
[ 1, 3, 5, 6, 9, 0, 2, 4, 6, 8 ]

Elekstube lixie (untested, data taken from https://github.com/KenReneris/EleksTubeClock
10 panes, 2 leds per digit (parallel), 10 leds per digit in the string
Digit number 0 on the left of the string ??
[ 5, 0, 6, 1, 7, 2, 8, 3, 9, 4 ]

Glixie lixie (untested, data taken from code in https://create.arduino.cc/projecthub/xingda/gixie-clock-7c129b
10 panes, 1 led per digit, 10 leds per digit in the string
Digit number 0 on the right of the string ??
[ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ]

Effects:

Transitional effects: These are only applied to digits that change between subsequent writes.
Changes between a digit and a space are also taken into account. Transitional effects are not 
taken into account for digits set to space by the clear_all() and clear_digit(i) funtions. 

1. INSTANT
When a digit changes between subsequent writes, the overwritten digit is instantly 
replaced by the written digit without any other effects

2. CROSSFADE
When a digit changes between subsequent writes, the overwritten digit slowly 
fades out from foreground color to the background color while the written digit 
slowly fades in from the background color to the foreground color. 
- trans_time controls duration of the crossfade. (nice setting: 200 ms, 800 ms)

3. DEEPFADE
When a digit changes between subsequent writes, the overwritten digit slowly
fades out from foreground color to the background color. Once it is completely 
faded out, the written digit starts to slowly fade in from background color to 
the foreground color. 
- trans_time controls duration of the deepfade (600 ms)

Color effects:





Write effects

Note:
For some effects it may be adviseable to surround them with a 'write("      ")'
to isolate them from other effects. Note that a 'write("     ")' differs
from a 'clear_all' ( the clear_all instantly clears the digits, while the writing
of spaces takes into accound the tarnsition type and transition color effects.

1.  INSTANT 
The intended value is written to the display in one go 
without any other effects. This effect is used a default when the 
trans_effect variable contains a non described effect number.
 
2.  ONEBYONELEFT 
The new value will be 'dropped' in from left  to right. 
thereby overwriting the previous indicated values. 
- trans_time will indicate the speed from left to right. (400 ms)

3.  ONEBYONERIGHT  
The new value will be 'dropped' in from right to left, 
thereby overwriting the previous indicated values. 
- trans_time will indicate the dropping speed. (400 ms)

4.  SCROLLLEFT 
The new value will be 'pushed' in from the left side, 
thereby 'pushing' forward the previous indicated values. 
- trans_time will indicate the pushing speed. (250 ms)

5.  SCROLLRIGHT
The new value will be 'pushed' in from the right 
side, thereby 'pushing' forward the previous indicated values. 
- trans_time will indicate the pushing speed. (250 ms)

6.  SCROLLFLAPLEFT 
Every lixie will 'flap' from its previous indicated value to its intended 
indicated value, they will flap at least one complete 0-9 round. 
They will change one by one from left to right.
- trans_time indicates the flapping speed. (40 ms)

7.  SCROLLFLAPRIGHT 
Every lixie will 'flap' from its previous indicated value to its intended 
indicated value, they will flap at least one complete 0-9 round. They will 
change one by one from right to left.
- trans_time indicates the flapping speed. (40 ms)   

8.  BLINKER 
after the intended value is written just like that (like 'INSTANT'), the 
display will blink 10 times. This blinking is part of the write action.
- trans_time will indicate the blinking speed. (150 ms)

9.  SLOTMACHINE 
Every lixie display is a reel on a slot machine. They spin and will stop on 
the number indicated in the call to this write function. The reel slows down 
like a real slot machine, next reel will slow down once previous reel stopped. 
- trans_time will indicate the spinning speed. (30 ms)

10. PINBALL 
Like an old pinballmachine with electromechanical score displays. In the first 
round, the previous value of the lixie is taken into acount. They start spining 
to zero when the counter is equal to their value (like the score reset on the 
pinball machine). Then lixies that do not need to display a number are cleared 
and the rest will take 2 complete 0-9 rounds in unison. In the third round 
the lixies will stick on their intended value. 
- trans_time will indicate the spinning speed. (130 ms)

11. GASPUMP  
Like an not-so-old gaspump with electromechanical 'flappy' 7segment displays. 
At start, all lixies that must display a digit will spin in unison (all numbers 
equal) After one full cycle, every next cycle the next display will stop at its 
intended value while the rest keeps spinning.
- trans_time will indicate the spinning speed. (60 ms)

12. SLOTIIMACHINE 
Variant on the slotmachine theme. Works like SLOTMACHINE, except next reel will 
slow down before previous reel stops. 
- trans time indicates the spinning speed. (40 ms)

Writing integers:
Write() can handle positive numbers and negative numbers,
even if you do not have an extra pane for the minus sign.
If a negative number is passed, the result will be prepended with an extra zero
to denote the minus sign.
if the value is 'out of bounds' the value is passed to the floatplus() function 
(which can handle bigger absolute values).
If (left side) padding is needed, it will be with spaces.

Writing floats:
Write_floatplus() displays the input as a float if needed in scientific notation, 
even if you do not have extra panes for the minus ('-') sign, the decimal poijnt ('.') and the 
exponent sign ('e').  It uses an extra leading zero for the minus sign, and uses
'dimming' for the digits after the decimal point but before the exponent.
If (left side) padding is needed it wil be spaces.
  
if input between 0 and 1e-09:        ERROR. display is "     0".
if input between 1e-09 and 0.000001: leading zero, 3 significant in dimmed color, 
                                     a zero and the (single digit) exponent 
if input between 0.000001 and 1:     leading zero and 5 decimals in dimmed color.
if input between 1 and 10:           leading digit plus 5 decimals in dimmed color.
if input between 10 and 100:         2 leading digits plus 4 decimals in dimmed color.
...
if input between 10000 and 100000:   5 leading digits plus one decimal in dimmed color.
if input between 100000 and 1000000: 6 leading digits.
if input between 1e06 and 1e10:      one leading digit, next 4 leading digits in dimmed color
                                     one digits for the exponent in the normal color.
if input between 1e10 and 1e100:     one leading digit, next 3 leading digits in dimmed color
                                     two digits for the exponen in the normal color.
if input bigger than 1e100 (googol): ERROR. 
                                     Display "999999" with the 9s in position 2-3-4 dimmed
if input == 0:                       displays ("000000") with positions 2-3-4-5-6 dimmed
if input between 0 and -1e-09:       ERROR. display is "     0".
if input between -1e-09 and -1e-04:  2 zeros, 2 dimmed significant decimals, 
                                     a zero and the (single digit) negative exponent 
if input between -0.0001 and -1:     2 zeros plus 4 dimmed decimals.
if input between -1 and -10:         a zero, the leading digit plus 4 dimmed decimals.
if input between -10 and -100:       a zero, 2 leading digits plus 3 dimmed decimals.
...
if input between -1000 and -10000:   a zero, 4 leading digits plus 1 dimmed decimal.
if input between -10000 and -100000: a zero and 5 leading digits.
if input between -1e05 and -1e10:    a zero, one leading digit, 3 dimmed decimals,
                                     one digits for the exponent (not dimmed).
if input between -1e10 and -1e100:   a zero, one leading digit, 2 dimmed decimals,
                                     two digits for the exponent (not dimmed).
if inptu smaller than -1e100:        ERROR. 
                                     Display "099999" with the 9s in position 3-4 dimmed

The argumment color is chosen for the color the lixies will light up. If a lixie is to be dimmed, 
(r,g,b) values of the color are divided by 4.

*/
