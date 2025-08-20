
#ifndef WEAS_FILTER_H
#define WEAS_FILTER_H

#include "common.h"
#include "app.h"

#include <cstddef>
#include <stdint.h>


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
    return next_or(in, 1, repeat);
  };

  uint16_t next_or(uint16_t in, uint16_t thresh, uint16_t repeat) {
    uint16_t out = next(in);
    if ((prev > out && prev - out < thresh) || (prev <= out && out - prev < thresh)) return repeat;
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
  uint16_t now = 0;
  uint16_t prev = 0;

private:
  uint16_t thresh;
  bool initialised = false;

};


// similar to above, but we reduce the threshold for the "current" knob
// also, trying to be fast so this can go in main loop

class Gate {

public:
  Gate(uint16_t lo, uint16_t hi);
  bool accumulate(size_t knob, uint16_t now, uint16_t prev);
  uint16_t now[KnobSpec::N_KNOBS] = {};
  uint16_t prev[KnobSpec::N_KNOBS] = {};

private:
  size_t active = KnobSpec::N_KNOBS;  // initially invalid
  uint16_t thresh_lo;
  uint16_t thresh_hi;
  bool retain[KnobSpec::N_KNOBS] = {};

};


// with oversampling, we can leave filtering to core 1 and combine the above

class KnobCleaner {

public:
  KnobCleaner(uint8_t lo, uint8_t hi);
  static constexpr uint16_t SKIP = 0xffff;
  uint16_t get(uint8_t knob, When when);
  bool append(uint8_t knob, uint16_t now, uint16_t prev);

private:
  uint8_t active = KnobSpec::N_KNOBS;
  uint8_t thresh_lo;
  uint8_t thresh_hi;
  uint16_t latest[N_WHEN][KnobSpec::N_KNOBS+1];
  MovingAverage<uint16_t, 3> average[N_WHEN][KnobSpec::N_KNOBS];

};

#endif

