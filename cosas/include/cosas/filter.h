
#ifndef WEAS_FILTER_H
#define WEAS_FILTER_H

#include <stdint.h>
#include <cstddef>


// https://cytomic.com/files/dsp/DynamicSmoothing.pdf

class SelfModLP {

public:
  SelfModLP(uint8_t bits, uint32_t sample_freq, uint32_t cutoff, float sensitivity);
  uint16_t next(uint16_t in);
  int16_t next(int16_t in);
  uint16_t next_or(uint16_t in, uint16_t same);
  int16_t next_or(int16_t in, int16_t same);

private:
  uint16_t max;
  uint16_t zero;
  uint16_t low1 = 0;
  uint16_t low2 = 0;
  float g0;
  float sense;

};


// WIDTH_BITS should < 5 for 12 bit values

template<size_t WIDTH_BITS> class MovingAverage {

public:

  MovingAverage() {};

  uint16_t next(uint16_t in) {
    for (size_t i = 0; i < WIDTH; i++) sums[i] += in;
    uint16_t out = sums[index & MASK] >> WIDTH_BITS;
    sums[index++ & MASK] = 0;  // setting to in would extend length by 1
    return out;
  };

  uint16_t next_or(uint16_t in, uint16_t repeat) {
    uint16_t out = next(in);
    if (prev == out) return repeat;
    prev = out;
    return out;
  };

private:
  static constexpr uint16_t WIDTH = 1 << WIDTH_BITS;
  static constexpr uint16_t MASK = WIDTH - 1;
  uint32_t index = 0;
  uint16_t prev = 0;
  uint16_t sums[WIDTH] = {};

};


#endif

