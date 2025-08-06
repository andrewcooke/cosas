
#include "cosas/filter.h"

#include <cmath>
#include <iostream>
#include <numbers>


SelfModLP::SelfModLP(uint8_t bits, uint32_t sample_freq, uint32_t cutoff, float sensitivity) {
  max = (1 << bits) - 1;
  zero = 1 << (bits - 1);
  float wc = cutoff / static_cast<float>(sample_freq);
  float gc = tanf(static_cast<float>(std::numbers::pi) * wc);
  g0 = 2 * gc / (1 + gc);
  sense = 4 * sensitivity;
  // std::cout << "gc = " << gc << std::endl;
 }

uint16_t SelfModLP::next(uint16_t in) {
  uint16_t low1z = low1;
  uint16_t low2z = low2;
  int16_t bandz = low1z - low2z;
  float g = std::min(1.0f, g0 + sense * std::abs(bandz));
  low1 = static_cast<uint16_t>(low1z + g * (in - low1z));
  low2 = static_cast<uint16_t>(low2z + g * (low1 - low2z));
  // std::cout << "1/2 " << low1 << " " << low2 << " g " << g << std::endl;
  return std::min(low2, max);
}

int16_t SelfModLP::next(int16_t in) {
  return static_cast<int16_t>(next(static_cast<uint16_t>(in + zero))) - zero;
}

uint16_t SelfModLP::next_or(uint16_t in, uint16_t same) {
  static uint16_t prev = 0;
  uint16_t next_ = next(in);
  if (next_ == prev) return same;
  prev = next_;
  return next_;
}

int16_t SelfModLP::next_or(int16_t in, int16_t same) {
  return static_cast<int16_t>(next_or(static_cast<uint16_t>(in + zero), static_cast<uint16_t>(same + zero))) - zero;
}

