
#include "cosas/filter.h"


ThresholdRange::ThresholdRange(uint16_t thresh) : thresh(thresh) {};


bool ThresholdRange::add(uint16_t n, uint16_t p) {
  if (initialised) {
    now = now + n - p;
  } else {
    now = n;
    prev = p;
    initialised = true;
  }
  // reset if we exceed the threshold and return true to indicate data available
  initialised = !((now > prev && (now - prev) > thresh) || (now < prev && (prev - now) > thresh));
  return !initialised;
}


Gate::Gate(uint16_t lo, uint16_t hi) : thresh_lo(lo), thresh_hi(hi) {};

bool Gate::accumulate(size_t knob, uint16_t n, uint16_t p) {
  if (retain[knob]) {
    now[knob] = now[knob] + n - p;
  } else {
    now[knob] = n;
    prev[knob] = p;
    retain[knob] = true;
  }
  uint16_t thresh = knob == active ? thresh_lo : thresh_hi;
  retain[knob] = !((now[knob] > prev[knob] && (now[knob] - prev[knob]) > thresh) || (now[knob] < prev[knob] && (prev[knob] - now[knob]) > thresh));
  if (!retain[knob]) active = knob;
  return !retain[knob];
}


KnobCleaner::KnobCleaner(uint8_t lo, uint8_t hi) : thresh_lo(lo), thresh_hi(hi) {};

bool KnobCleaner::append(uint8_t knob, uint16_t now, uint16_t prev) {
  if (knob < N_KNOBS) {
    latest[Prev][knob] = average[Prev][knob].next(prev);
    uint16_t thresh = knob == active ? thresh_lo : thresh_hi;
    latest[Now][knob] = average[Now][knob].next_or(now, thresh, SKIP);
    if (latest[Now][knob] != SKIP) {
      active = knob;
      return true;
    } else {
      return false;
    }
  } else {
    // switch!
    active = knob;
    latest[Prev][knob] = prev;
    latest[Now][knob] = now;
    return true;
  }
}

uint16_t KnobCleaner::get(uint8_t knob, When when) {
  return latest[when][knob];
}


