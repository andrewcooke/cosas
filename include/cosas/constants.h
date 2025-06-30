
#ifndef COSA_CONSTANTS_H
#define COSA_CONSTANTS_H

#include <limits>
#include <cstdint>
// ReSharper disable once CppUnusedIncludeDirective
#include <cstddef>  // needed for size_t


constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint8_t SAMPLE_BITS = 16;
constexpr int16_t SAMPLE_MAX = std::numeric_limits<int16_t>::max();
constexpr int16_t SAMPLE_MIN = -SAMPLE_MAX;  // throw away one value because i don't like the asymmetry

// leads to omega = f (see source.h)
constexpr size_t FULL_TABLE_SIZE = SAMPLE_RATE;
constexpr size_t HALF_TABLE_SIZE = SAMPLE_RATE / 2;
constexpr size_t QUARTER_TABLE_SIZE = SAMPLE_RATE / 4;

constexpr uint8_t SUBTICK_BITS = 3;
constexpr size_t FULL_TABLE_SUB = FULL_TABLE_SIZE << SUBTICK_BITS;

// see discussion in oscillator.cpp
constexpr size_t PHI_FUDGE_BITS = 12;


#endif
