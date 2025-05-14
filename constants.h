
#ifndef COSA_CONSTANTS_H
#define COSA_CONSTANTS_H

import std;
using namespace std;

#include <cstdint>


const uint32_t sample_rate = 44100;
const int sample_depth = 16;
const uint16_t sample_zero = 1 << (sample_depth - 1);
const uint16_t sample_max = numeric_limits<uint16_t>::max();

// half here refers to the +ve or -ve going waveform relative to
// sample_zero.  so 16 bits sample size has 15 bits + ve and 15 -ve.
const int half_depth = sample_depth - 1;
const uint16_t half_max = sample_max >> 1;

// leads to omega = f (see source.h)
const size_t full_wavetable_size = sample_rate;
const size_t full_lookup_size = 1 << (sample_depth / 2);

#endif
