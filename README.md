## fixed code for dimmers from  ulrich-radig.de

#### intro:
in the 3 and 8 channel projects there are fixed 
dimmercurves from the forum named ...SKALIERT.c , 

( with the original calculation , dimming starts from ~35 % or dmx value 75 , this is also the point ehere voltage drops below 10v)

Since 
> (255-DMX_VAL)/2

will be max 128 or 127 , and the dimmmer has 90 possible values from phase cutting , the transformation suggested in his forum is applied via valMap

#### install:
-> atmel studio 7 -> new project -> c++ -> stk5000 (find com .. ) -> atmega328p -> insert sourceode -> build solution (f7) -> "tools→add target" -> "tools→device programming" -> "read"(top) ->memories (check for build to appear) -> erase flash -> program 
