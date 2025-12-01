
# Sampling

Notes on calculation of sampling frequencies, buffers, etc (xqria is
*not* a sampler!).

## Context

This is a rewrite of an [earlier document](sampling-old.md) with a
more clearly thought out set of values.

My main problem is that I appear to have noise at hugh frequencies
that is aliasing into the audible range, because the hardware doesn't
have a filter.  And one simple way to address this is to oversample.
