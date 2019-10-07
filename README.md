## fixed code for dimmers from  ulrich-radig.de

#### intro:
in the 3 and 8 channel projects there are fixed 
dimmercurves from the forum named ...SKALIERT.c , 

( with the original calculation , dimming starts from ~35 % or dmx value 75 , this is also the point where voltage drops below 10v)

Since 
> (255-DMX_VAL)/2

will be max 128 or 127  and the dimmmer has 90 possible values from phase cutting ,
 the transformation is done as suggested in his forum[1] via valMap

hint: the values are reversed 100%=0 ,0%=90 internal 

#### install:
-> atmel studio 7 -> new project -> c++ -> stk5000 (find com .. ) -> atmega328p -> insert sourceode -> build solution (f7) -> "tools→add target" -> "tools→device programming" -> "read"(top) ->memories (check for build to appear) -> erase flash -> program 



[1]  https://www.ulrichradig.de/forum/viewtopic.php?f=53&t=2656s
