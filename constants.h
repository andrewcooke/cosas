
#ifndef FMCOSA_CONSTANTS_H
#define FMCOSA_CONSTANTS_H

#include <cstdint>


const uint32_t sample_rate = 44100;
const uint32_t table_size = sample_rate / 4;
const int bit_depth = 16;
const uint16_t sample_zero = 1 << (bit_depth - 1);


#endif
