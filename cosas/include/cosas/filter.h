
#ifndef WEAS_FILTER_H
#define WEAS_FILTER_H

#include <cstddef>
#include <cstdint>
#include <array>

#include "cosas/common.h"
#include "cosas/ctrl.h"
#include "cosas/debug.h"


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

  uint16_t next_or(uint16_t in, uint16_t thresh, uint16_t repeat) {
    uint16_t out = next(in);
    if (index < WIDTH) {
      prev = out;
      return repeat;
    }
    if ((prev > out && prev - out < thresh) || (prev <= out && out - prev < thresh)) {
      return repeat;
    }
    // BaseDebug::log("index", index, "prev", prev, "out", out, "thresh", thresh);
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
  uint16_t now[N_KNOBS] = {};
  uint16_t prev[N_KNOBS] = {};

private:
  size_t active = N_KNOBS;  // initially invalid
  uint16_t thresh_lo;
  uint16_t thresh_hi;
  bool retain[N_KNOBS] = {};

};


// with oversampling, we can leave filtering to core 1 and combine the above

// this gates knob values based on a moving average.  the gate threshold
// is lower for the "current" knob.  the idea is that the thresholds for
// the non-current knob are high enough to block noise, but once a knob is
// active smaller movements will be allowed through.

// switch is special cased and always passed.

// to use, call test() with the latest data.  if this returns true then
// retrieve the filtered data with get().

class CtrlGate {

public:
  CtrlGate(std::array<uint16_t, N_KNOBS> lo, std::array<uint16_t, N_KNOBS> hi);
  static constexpr uint16_t SKIP = 0xffff;
  CtrlEvent get();
  bool test(CtrlEvent event);

private:
  std::array<uint16_t, N_KNOBS> thresh_lo;
  std::array<uint16_t, N_KNOBS> thresh_hi;
  CtrlEvent active = CtrlEvent(CtrlEvent::Switch, 0, 0);
  MovingAverage<uint32_t, 1> average[N_WHEN][N_KNOBS];

};

#endif

