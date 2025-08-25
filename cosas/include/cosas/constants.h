
#ifndef COSAS_CONSTANTS_H
#define COSAS_CONSTANTS_H

#include <cmath>
#include <limits>
// ReSharper disable once CppUnusedIncludeDirective
#include <cstdint>

constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint8_t SAMPLE_BITS = 16;
constexpr int16_t SAMPLE_MAX = std::numeric_limits<int16_t>::max();
constexpr int16_t SAMPLE_MIN =
    -SAMPLE_MAX; // throw away one value because i don't like the asymmetry

constexpr size_t FULL_TABLE_SIZE = SAMPLE_RATE;
constexpr size_t HALF_TABLE_SIZE = SAMPLE_RATE / 2;
constexpr size_t QUARTER_TABLE_SIZE = SAMPLE_RATE / 4;

constexpr uint8_t SUBTICK_BITS = 3;
constexpr size_t FULL_TABLE_SUB = FULL_TABLE_SIZE << SUBTICK_BITS;

constexpr size_t PHI_FUDGE_BITS = 12;

inline size_t tick2idx(int32_t tick) {
  return (tick >> SUBTICK_BITS) % FULL_TABLE_SIZE;
}

inline uint32_t hz2freq(const float hz) {
  auto freq = static_cast<uint32_t>(std::fabsf(hz) * (1 << SUBTICK_BITS));
  freq = std::min((SAMPLE_RATE / 2) << SUBTICK_BITS,
                  std::max(static_cast<uint32_t>(1u), freq));
  return freq;
}

inline float freq2hz(const uint32_t freq) {
  return static_cast<float>(freq) / (1 << SUBTICK_BITS);
}

#endif
