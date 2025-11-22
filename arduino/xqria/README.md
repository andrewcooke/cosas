
# xqria - a euclidean pattern generator

## features

* two euclidean patterns
* two voices per pattern
* each voice has volume, frequency, tone, duration and swing
* 14 memories to save patterns
* reverb
* simple (breadboard-able), low price electronics using esp32
* free, extensible software, with buffering and separate gui thread

## examples

<audio controls>
  <source src="audio/untitled7.mp3" type="audio/mpeg">
</audio>

https://github.com/andrewcooke/cosas/blob/main/arduino/xqria/audio/unititled7.mp3

## user interface

the ui assumes that the leds, pots, and buttons are arranged into 4
"control groups", each containing one led, one button, and one pot.
here is an example of what they may look like:



## building xqria

the main component is an esp32 development board, with audio output to
dac0.  in addition, the software requires 4 buttons (pullup), 4 leds,
and 4 potentiometers connected to free pins (the pin numbers can be
changed in the code).

there should be a clear progression between the control groups (maybe
placing them from left to right in a horizontal line, or by numbering
them from 1 to 4, for example).

i developed the project using the "proto-synth" from gclab -
https://gclabchile.com/ - and i recommend contacting gonzalo for more
information if you want to do the same, but i don't use any of the
other proto-synth features and the description above may be sufficient
for experienced developers to roll their own.

## installing xqria

once you have the board working the "ino" file in this directory can
be loaded into an arduino ide and written via usb to the board.

you may need to edit the pins to match your hardware - see "PINS"
comments in code.  the pins are ordered in control group order, so
the first button pin is in the same group as the first led pin and the
first pot pin, forming the first control group.

## using xqria

### voices

to edit a voice, press the corresponding button and then turn a pot.
for example, to adjust voice 1 parameters, you press button 1.  the
different pots control different parameters:

1. volume (split in two - the first half has a sharper attack)
2. frequency / pitch
3. duration
4. tone (split into drums, noise/cymbals, and squelchy fm beeps)

#### catch-up

if you play with the voice settings you'll soon realise that there is
something odd in how they work.  initially, moving the pot doesn't
change anything.  it may not be clear at first, but the pot only
starts to take effect when it arrives at the position of the current
value.

this is actually very useful because the pots are used to adjust many
parameters.  for example, amongst many other things, the first pot
controls the volume for all the voices.  if you have just turned voice
3 as loud as possible and then select voice 1 you might not want that
to jump to the same loud value.  so the pot automatically waits until
you are close to the current value before "turning on".

many operations are associated with patterns of lights on the leds.
in general these lights will start dim, showing the current setting
for a parameter, and only become bright when the pot has "caught up"
with the current value.

### patterns

a euclidean pattern has two main parameters: the total length, in
beats, and the number of times the voice will trigger during those
beats.  the "euclidean" part spreads the triggers out as evenly as
possible.

so, for example, if you have 4 beats and 2 triggers, the triggers will
be on the first and third beats.

xqria has two patterns.  one pattern controls voices 1 and 2; the
other controls voices 3 and 4.  to adjust parameters associated with a
pattern hold down the buttons correspodning to the voices in that
pattern.

with buttons 1 and 2 held down, for example, to select the first
pattern, the pots then adjust:

1. the total number of beats
2. the number of triggers (as a fraction of the number of beats)
3. the division in work between the two voices
4. the probability that some triggers shift which beat they occur on

pots 1 and 2 use the leds in an unusual way: the leds correspond to
the factors 2, 3, 5 and 7.  so if the total number of beats is 30, for
example, then leds 1, 2, and 5 will be lit (because 30=2x3x5).  if the
number of beats is 16 then only the first led will be lit, because all
the factors of 16 are 2 (16=2x2x2x2).

this makes sense (honestly!) because more traditional, regular
patterns, tend to be associated with factors of 2, while other factors
give longer, more varied patterns.  so the leds are not just showing
maths, they are an indication of how complex the resulting rhythm will
be (for extra variety, select combinations where the total number of
beats and the number of triggers don't have the same factors.  values
when no leds are lit - primes larger than 7 - give particularly long
patterns).

### timing (TODO - rename in code from global)

holding down the central buttons (2 and 3) gives accesss to timing
parameters.  the pots adjust:

1. overall bpm
2. 

### post-processing

### performance (TODO - better name?)

### editing

## support

feel free to contact me at andrew@acooke.org or by creating issues
here.

*hecho en chile con amooooor*
