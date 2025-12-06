
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

Considering bit depth (not table size), the lookup table for sine is
amplitude, not frequency, but it seems reasonable that we want a
similar dynamic range for FM modulation.  And if I am going to use
more than 8 bits I might as well use 16 for no penalty over 10
(packing two 12 bit values into 3 bytes is tricky because it involves
factors of 3, not 2, and that complexity doesn't seem worthwhile).

So, for simplicity, with the possibility of reducing noise, and no
real cost I will use 16 bits for frequency and amplitude values
internally, where 0xffff corresponds to the maximum value and inputs
and oututs are shifted accordingly.

## (Base) Sampling Frequency

I will split sampling frequency into a base frequency and
an oversampling factor.

Base frequency could be 40, 44 or 48kHz.  Since we're not
inter-operating with anything digitally, let's choose 40kHz.

For oversampling, it's probaby simplest to describe that in bits,
since it allows for bitshift conversion.

## Lookup Table Width

This should be a power of 2 so that we can shift max frequency (20kHz)
to correspond to half table size.  In practica only a quarter table is
needed.  The exact value can be tuned on compiling/listening (along
with oversample bits).

## Oversampling

If we oversample then phase increments need to be reduced
appropriately, which is equivalent to shifting frequency down by
oversample bits.

This argues for a progressively larger lookup table to avoid repeated
values at low frequencies.