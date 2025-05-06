
#ifndef FMCOSA_CONSTANTS_H
#define FMCOSA_CONSTANTS_H

#include <cstdint>

const uint32_t sample_rate = 44100;
const uint32_t table_size = sample_rate >> 2;
const uint16_t zero_bits = 1 << 15;

#endif
