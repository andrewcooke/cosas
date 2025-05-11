
#ifndef COSA_CONSTANTS_H
#define COSA_CONSTANTS_H

import std;
using namespace std;

#include <cstdint>


const uint32_t sample_rate = 44100;
const int sample_depth = 16;
const uint16_t sample_zero = 1 << (sample_depth - 1);
const uint16_t sample_max = numeric_limits<uint16_t>::max();

// leads to omega = f (see source.h)
const uint16_t full_table_size = sample_rate;

const uint8_t max_oscillators = 6;

#endif
