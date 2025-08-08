
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


template<size_t WIDTH_BITS> class MovingAverage {

public:

  MovingAverage(uint16_t same) : same(same) {};

  uint16_t next(uint16_t in, size_t count) {
    for (size_t i = 0; i < WIDTH; i++) weights[i] += in;
    uint16_t out = weights[count & MASK] >> WIDTH_BITS;
    weights[count & MASK] = 0;
    return out;
  };

  uint16_t next_or(uint16_t in, size_t count) {
    static uint16_t prev = same;
    uint16_t out = next(in, count);
    if (prev == out) return same;
    prev = out;
    return out;
  };

private:
  static constexpr uint16_t WIDTH = 1 << WIDTH_BITS;
  static constexpr uint16_t MASK = WIDTH - 1;
  uint16_t same;
  uint16_t weights[WIDTH] = {};

};


#endif

