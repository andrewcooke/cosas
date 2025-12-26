
# Sampling

Notes on calculation of sampling frequencies, buffers, etc (xqria is
*not* a sampler!).

## Context

This is a rewrite of an [earlier document](sampling-old.md) with a
more clearly thought out set of values.

My main problem is that I appear to have noise at high frequencies
that is aliasing into the audible range, because the hardware doesn't
have a filter.  And one simple way to address this is to oversample.

## Frequency and Lookup Table Depth

The audio range from 20Hz to 20kHz needs a factor of 1000, or 10 bits.

In fact, 10 bits for frequency isn't great because resolution at low
frequencies is very low (20Hz then 40Hz).  So it's better to use 12
bits, ie all that is available from a single pot.

Considering bit depth (and not table size), the lookup table for sine
is amplitude, not frequency, but it seems reasonable that we want a
similar dynamic range for FM modulation.  And if I am going to use
more than 8 bits I might as well use 16 for no penalty over 10
(packing two 12 bit values into 3 bytes is tricky because it involves
factors of 3, not 2, and that complexity doesn't seem worthwhile).

So, for simplicity, with the possibility of reducing noise, and no
real cost I will use 16 bits for frequency and amplitude values
internally, where 0xffff corresponds to the maximum value and inputs
and outputs are shifted accordingly.

## Phase (Tau)

While generating audio we accumulate phase.  This loops at the
equivalent of 2 pi.  If the highest frequency (20kHz) uses 16 bits
then we need to accumulate over 17 bits, because we need to sample
twice in a single cycle.

In other words, by selecting frequency as 16 bits we have defined 2 pi
(aka tau) as 17 bits.

## (Base) Sampling Frequency

I will split sampling frequency into a base frequency and
an oversampling factor.

Base frequency could be 40, 44 or 48kHz.  Since we're not
inter-operating with anything digitally, let's choose 40kHz.

For oversampling, it's probably simplest to describe that in bits,
since it allows for bit-shift conversion.

## Lookup Table Width

This should be a power of 2 so that we can shift max frequency (20kHz)
to correspond to half table size.  In practice only a quarter table is
needed.  The exact value can be tuned on compiling/listening.

Note that if the table width is not equal to tau then we need to
down-sample the phase before lookup.  It is not clear to me whether
this will have a noticeable effect on the sound (it seems like it will
increase the noise floor).  So, again, I need to experiment with
different table sizes.

## Oversampling

If we oversample then phase increments need to be reduced
appropriately, which is equivalent to shifting frequency down by
oversample bits.

But this leads to a loss of resolution, which might affect sound
quality (especially for percussion, which uses extreme high and low
frequencies).

An alternative to down-shifting frequency is to increase tau by a
corresponding amount.  This will also affect either the lookup table
width or, more likely, the down-sampling needed on lookup.

Note added later: experiments show that only 1 bit of oversampling is
possible with the current code (we cannot fill the buffer in time when
all 4 voices are active).  When development is (largely) complete it
would be worth seeing if performance tuning could extend this to 2
bits.

## Frequency Lookup

After all the above, I still wasn't happy with frequency range and so
considered using a lookup table from knob value to note frequency,
with note frequency pre-calculated.  This means that the generated
signals will be (1) exponentially distributed and (2) in "correct"
tune (ignoring FM).

By "note frequency" I mean the phase increment necessary to generate a
particular frequency.

To make this work I need to calculate the audio frequency, given the
phase increment.  Remember that the base frequency is 40Hz and tau is
17 bits.  A step of tau generates the base frequency, so, scaling:

    f = BASE x step / (2 ^ tau)

And inverting for step size:

    step = f x (2 ^ tau) / BASE

