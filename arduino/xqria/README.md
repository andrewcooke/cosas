
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

TODO

## hardware

the main component is an esp32 development board, with audio output to
dac0.  in addition the design assumes 4 buttons (pullup), 4 leds, and
4 potentiometers connected to free pins (the pin numbers can be
changed in the code).

the gui assumes that the leds, pots, and buttons are arranged into 4
"control groups", each containing one led, one button, and one pot.
also, there should be a clear progression between the control groups
(eg by placing them from left to right in a horizontal line).

i worked with the "proto-synth" designed by gclab -
https://gclabchile.com/ - and i am sure that gonzalo would be happy to
cooperate with any hardware, i don't use any of the other proto-synth
features and believe the description above is probably enough to roll
your own.

## installation

once you have the board working the "ino" file in this directory can
be loaded into the arduino ide and written via usb to the board.  you
may need to edit the pins before upload (see "PINS" comments in code).

the pins are ordered in control group order.  so the first button pin
is in the same group as the first led pin and the first pot pin,
forming the first control group.

## user interface

```

```

## instructions

### voices

### patterns

### timing (TODO - rename in code from global)

### post-processing

### performance (TODO - better name?)

### editing

## support

feel free to contact me at andrew@acooke.org or by creating issues
here.

*hecho en chile con amooooor*
