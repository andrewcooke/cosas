
#ifndef WEAS_FILTER_H
#define WEAS_FILTER_H

#include <stdint.h>
#include <cstddef>


// WIDTH_BITS should < 5 for 12 bit values and uint16_t
// for larger WIDTH_BITS using uint32_t

template<typename INT, size_t WIDTH_BITS> class MovingAverage {

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
  INT sums[WIDTH] = {};

};


// the idea here is that we accumulate until the difference between next and
// prev is large enough to be significant.  at that point add() returns true
// and the attributes should be used.

class ThresholdRange {

public:
  ThresholdRange(uint16_t thresh);
  bool add(uint16_t now, uint16_t prev);
  uint16_t now;
  uint16_t prev;

private:
  uint16_t thresh;
  bool initialised = false;

};


#endif

