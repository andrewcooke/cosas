
# Sampling

Notes on calculation of sampling frequencies, buffers, etc (xqria is
*not* a sampler!).

## Context

Now that I have things working "well" -

 * separate UI thread

 * asynchronous sound creation to a buffer

 * offloaded dma to feed dac

- it turns out that I have enough "free" cpu to oversample
considerably.

This is useful because, at 40kHz, I was getting a lot of unpleasant
noise when trying to emulate cymbals.  An oscilloscope showed that my
"sine" waves at around 1kHz had a very pronounced "stepped" appearance
(something which, on reflection, is not surprising - there is no low
pass filtering after the output).

This super-audio structure meant that what I heard depended a *lot* on
the audio chain I used.  And moving from single-cone speakers fed
through valves to monitoring headphones was an unpleasant shock.  A
higher sampling rate might (should?) move quantization noise to higher
and make listening more predictable (and pleasant).

## Choice of Basis

There are lots of inter-related variables in the code.  Deciding which
are fundamental, and which derived, affects how easily the code can be
adapted.  Here I explain my choices and show how those affect derived
parameters.

The basis values I use are:

  SAMPLE_FREQ_HZ - self explanatory, eg 40kHz
  LOWEST_F_HZ    - the lowest freq that is completely sampled, eg 20Hz
  PHASE_EXTN     - additional bits used to track phase

See "Table Depth" for an argument that this might not be optimal - I
could replace LOWEST_F_HZ with TABLE_DEPTH.

## Lookup Table

A lookup table that completely samples LOWEST_F_HZ at SAMPLE_FEQ_HZ
has a length of

  N_SAMPLES = SAMPLE_RATE_HZ / LOWEST_F_HZ

(by completely samples, I mean that at the lowest frequency, every
sample is a distinct lookup, so we need to divide one period of the
low frequency by the sampling period).

In practice, the actual table can be one quarter this size for sine
waves, from symmetry.

## Lookup (Frequency Units)

The lookup table is accessed using code like (ignoring PHASE_EXTN for
the moment):

  phase += frequency
  out = lookup(phase % N_SAMPLES)

Here a unit increment will correspond to LOWEST_F_HZ (see logic
above).  So the units of frequency are LOWEST_F_HZ (again, ignoring
PHASE_EXTN).

## Phase Extension

If phase and frequency are integers (for efficiency) then the lowest
frequencies are multiples of LOWEST_F_HZ and, therefore, widely
spaced.

To give higher resolution of low frequencies we can use use additional
bits to track phase which are discarded for lookup.

Potentionmeters for ESP32 return 12 bit values, while the difference
between 20Hz and 20kHz is a factor of 1000, which is close to 10 bits.
So if we use a phase extension of two bits then we can "naturally" use
the 12 bit potentiometer value exacly:

  LOWEST_F_HZ = 20
  PHASE_EXTN = 2
  FREQ_UNITS = LOWEST_F_HZ >> PHASE_EXTN = 5
  MAX_FREQ = FREQ_UNITS x (1 << 12) = 20kHz

This argument implies that PHASE_EXTN > 2 is not useful (if we use a
single pot to specify frequency).

## Table Depth

Here I will consider a quarter lookup table.

It is wasteful to have a table where values are repeated, so from a
pigeonhole argument values should be at least log2(N_SAMPLES/4) in
length.

  SAMPLE_RATE_HZ = 40,000
  N_SAMPLES = 2,000
  TABLE_DEPTH >= 11

Alternatively, one can argue that the table is too large if the depth
inferred above is not necessary in calculations.  This approach
suggests that starting from LOWEST_F_HZ was not optimal.

Output is 8 bits (7 bits unsigned).  So do we need 12 bits?  Arguably
it is useful for accurate FM synthesis (remmeber that 12 bits is the
resolution we use for frequency).  But this is somewhere that could be
optimized to reduce memory use (especially since with TABLE_DEPTH = 8
the samples can be uint8_t).

## Oversampling

Rather than varying SAMPLE_RATE_HZ I will introduce OVERSAMPLE_FACTOR.

The following values scale proportional to OVERSAMPLE_FACTOR:

  SAMPLE_RATE_HZ
  N_SAMPLES
  TABLE_DEPTH (as log2)

The following values are independent of OVERSAMPLE_FACTOR:

  LOWEST_F_HZ  by defn
  PHASE_EXTN   by defn
  FREQ_UNITS

The last - FREQ_UNITS - might be surprising.  At frst glance it seems
like it should scale with N_SAMPLES.  But in fact the increased in
SAMPLE_RATE_HZ exactly matches that, leaving FREQ_UNITS unchanged
(each frequency jump is *relatively* smaller, but more are needed per
second).

Note that if the TABLE_DEPTH replaces LOWEST_F_HZ as a basis then
FREQ_UNITS will change with OVERSAMPLING_FACTOR.

## Conclusion

There seem to be two "sweet spots".

* For 12 bit consistency, 2x OVERSAMPLING, 12 bit TABLE_DEPTH.

* For low memory use, TABLE_DEPTH of 8 bits, and a change of basis
  choices.

Also, higher OVERSAMPLING may justify larger TABLE_DEPTH, if memory is
not an issue.
