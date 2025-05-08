
#ifndef FMCOSA_CONSTANTS_H
#define FMCOSA_CONSTANTS_H

#include <cstdint>


const uint32_t sample_rate = 44100;
const int bit_depth = 16;
const uint16_t sample_zero = 1 << (bit_depth - 1);

// leads to omega = f (see source.h)
const uint16_t full_table_size = sample_rate;

#endif
