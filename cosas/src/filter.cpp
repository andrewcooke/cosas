
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


CtrlGate::CtrlGate(std::array<uint16_t, N_KNOBS> lo, std::array<uint16_t, N_KNOBS> hi) : thresh_lo(lo), thresh_hi(hi) {};

bool CtrlGate::test(CtrlEvent event) {
  if (event.ctrl == CtrlEvent::Switch) {
    active = event;
    return true;
  } else {
    uint16_t thresh = event.ctrl == active.ctrl ? thresh_lo[event.ctrl] : thresh_hi[event.ctrl];
    CtrlEvent filtered = CtrlEvent(event.ctrl,
      average[Now][event.ctrl].next_or(event.now, thresh, SKIP),
      average[Prev][event.ctrl].next(event.prev));
    if (filtered.now != SKIP) {
      active = filtered;
      return true;
    } else {
      return false;
    }
  }
}

CtrlEvent CtrlGate::get() {
  return active;
}

