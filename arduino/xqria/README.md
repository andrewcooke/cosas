
# xqria - a euclidean pattern generator

## features

* two euclidean patterns
* two voices per pattern
* four engines (two "drums" and two "cymbals") per voice, each with
  adjustable volume, pitch, tone, duration and swing
* compression and reverb
* 14 memories to save configurations
* simple (breadboard-able), low price electronics using esp32
* free, extensible software, with buffering and separate gui thread

## examples

(soundcloud here)

## hardware

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

## install

once you have the board working the "ino" file in this directory can
be loaded into an arduino ide and written via usb to the board.

you should edit the platform.txt (mine is in
/home/andrew/.arduino15/packages/esp32/hardware/esp32/3.3.3/platform.txt
for example) to have

  ````
  compiler.optimization_flags=-O3
  compiler.optimization_flags.release=-O3
  ````

(this optimises the code to reduce the chance of noise spikes).

finally, you may need to edit the pins to match your hardware - see
"PINS" comments in code.  the pins are ordered in control group order,
so the first button pin is in the same group as the first led pin and
the first pot pin, forming the first control group.

## user manual

### user interface

the ui assumes that the leds, pots, and buttons are arranged into 4
"control groups", each containing one led, one button, and one pot.
in the description below label these as 1 to 4, from left to right.

### voices

to edit a voice, hold down the corresponding button and then turn a
pot.  for example, to adjust voice 1 parameters, you hold button 1
down.  the different pots control different parameters:

1. volume (split in two to select an engine)
2. pitch (split into two to select an engine)
3. duration
4. tone

both volume and pitch are split into two ranges.  pitch chooses
between "drums" (low) and "cymbals" (high).  volume choose between
bass/snare (drums) and ride/crash (cymbals).  so there a total of four
voice engines to play with.

#### catch-up

if you play with the voice settings you'll soon realise that there is
something odd in how they work.  initially, moving the pot doesn't
change anything.  it may not be clear at first, but the pot only
starts to take effect when it arrives at the position of the current
value.

this is a useful feature, not a bug, because the pots are used to
adjust many parameters.  for example, amongst many other things, the
first pot controls the volume for all the voices.  if you have just
turned voice 3 as loud as possible and then select voice 1 you might
not want that to jump to the same loud value.  so the pot
automatically waits until you are close to the current value before
"turning on".

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
pattern hold down the buttons corresponding to the voices in that
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

holding down the central buttons (2 and 3) gives access to timing
parameters.  the pots adjust:

1. overall bpm

2. subdivision.  this controls the BPM for the second rhythm (voices 3
   and 4), relative to the first.  the default value is 1 (same
   speed); available values are expressed as multiples of 1/60.  the
   available multiples are 5, 10, 12, 15, 20, 24, 25, 30, 35, 36, 40,
   45, 48, 50, 55, and 60.

3. pattern offset.  this is the offset between the two patterns, in
   number of beats in the first.

the way subdivision is calculated is, unfortunately, complicated.  you
may prefer to simply adjust things by ear, but here i will explain in
more detail what is happening.

first, the subdivision if the fraction of a beat in the first pattern
that corresponds to a beat for the second pattern.  so a subdivision
of 30/60 means the second pattern runs twice as fast (it cycles twice
for each cycle of the first pattern), a value of 5/60 means 12x as
fast, and a value of 55/60 means that it runs only slightly faster, so
that the second pattern cycles 11 times for every 10 cycles of the
first.

second, the leds show the multiple (5, 10, 12 etc, as above) as
combinations of the prime factors 2, 3, 5 and 7.  so a multiple of 5
means that the third led is lit, a multiple of 24 (=2x2x2x3) lights
the first and second leds, etc.

### post-processing

the outer buttons control reverberation, compression and quantisation.
the pots adjust:

1. reverb delay.

2. balance between the delay and the main output.

3. degree of compression.

4. degree of quantisation (reduce the number of bits).

### performance

the left three buttons (1, 2 and 3) give access to performance
parameters.  the pots adjust:

1. which voices are enabled.  this is very useful when designing
   sounds and rhythms, as you can work on just one, or a pair, of
   voices.

2, 3 and 4.  individual shifts for voices 2, 3 and 4 (relative to
voice 1).  this allows for fine-tuning of the relative timing
(allowing "swing" etc).

### save

the right three buttons (2, 3 and 4) allow voices to be copied and the
entire configuration saved and restored.  the pots control:

1. selects a voice to copy

2. selects destinations for copying.  the selected voice (see above)
   is copied when the buttons are released.  if you change your mind
   and don't want to copy, then select no destinations and release the
   buttons.

3. read settings from memory.  the available memory slots are numbered
   1 through 14 and that number is shown in binary using the leds.

4. save the current settings to memory.  again, the memory slots are
   numbered 1 to 14.

memory 15 works a little like "trash" on a computer desktop - it
contains the overwritten settings when reading from memory, or the
previously slot settings when writing.  with careful use this can let
you move settings around without losing anything.

the memory is NOT preserved if the system is re-deployed.

### system

holding down all buttons gives access to some internal settings.  you
should not need to alter these unless experimenting with glitches or
similar.

1. buffer size.  this is the size (in samples) of the internal buffer
   that contains a fragment of sound before it is passed to the dac.
   the default is the largest buffer possible.  the two ranges
   correspond to event and odd buffer sizes (odd is not recommended).

2. oversample bits.  this is relative to the base clock of 40khz.  by
   default this is 2, which means we 4x oversample (160khz).  however,
   the cpu may not be able to generate complex sounds sufficiently
   quickly, in which case this can be reduced.  to monitor, set
   DBG_TIMING and look at "errors" and "late" in the serial output
   (there will likely be audible problems too).

   a value of 3 bits is supported for curiousity / glitching.

### cpu use

the entire instrument is running on a tiny chip with two cores.  if
you push the limits of what is possible you may exhaust the cpu.  and
if that happens you will start to hear noise/clicks in the audio.

if you are having problems, please check you have compiled with -O3
as described in the "install" section above, since that makes a huge
difference in avoiding overloading the cpu.

if you still have problems, you can reduce the oversample rate (see
system above), but this may reduce sound quality.

if you still have problems, you need to reduce the complexity of the
sound.  it's probably sufficient to reduce the duration of a few
voices, or reduce use of the ride cymbal (the most expensive sound to
calculate).  you could also, for example, silence a pair of voices
(EXPLAIN HOW HERE), record that, and then swap to the other two voices
and record that.

but please don't be alarmed!  this doesn't happen in "normal" use.  i
mention it here to be fully transparent, and because the behaviour is
a deliberate design choice - i could have made things so simple they
would never starve, but that would have made a much less interesting
machine.

to explore further, compile with DBG_TIMING set to true and look at
the output in the serial monitor.  if the "duty" value exceeds 100
(percent) then audio quality is affected.

## support

feel free to contact me at andrew@acooke.org or by creating issues
here.

*hecho en chile con amooooor*
