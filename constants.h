
#ifndef COSA_CONSTANTS_H
#define COSA_CONSTANTS_H

#include <cstddef>
#include <limits>
#include <cstdint>


const uint32_t sample_rate = 44100;
const uint8_t sample_bits = 16;
const int16_t sample_max = std::numeric_limits<int16_t>::max();
const int16_t sample_min = -sample_max;  // throw away one value because i don't like the asymmetry

// leads to omega = f (see source.h)
const size_t full_table_size = sample_rate;
const size_t half_table_size = sample_rate / 2;
const size_t quarter_table_size = sample_rate / 4;

const uint8_t subtick_bits = 3;

// see discussion in oscillator.cpp
const size_t phi_fudge_bits = 8;

#endif
