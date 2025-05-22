
#ifndef COSA_CONSTANTS_H
#define COSA_CONSTANTS_H

#include <cstddef>
#include <limits>
#include <cstdint>


const uint32_t sample_rate = 44100;
const uint8_t sample_depth = 16;
const int16_t sample_max = std::numeric_limits<int16_t>::max();
const int16_t sample_min = -sample_max;  // throw away one value because i don't like the asymmetry

// half here refers to the +ve or -ve going waveform relative to
// sample_zero.  so 16 bits sample size has 15 bits + ve and 15 -ve.
//const int half_depth = sample_depth - 1;
//const uint16_t half_max = sample_max >> 1;

// leads to omega = f (see source.h)
const size_t full_table_size = sample_rate;
const size_t half_table_size = sample_rate / 2;
const size_t quarter_table_size = sample_rate / 4;
//const size_t full_lookup_size = 1 << (sample_depth / 2);

const uint8_t subtick_bits = 3;

#endif
