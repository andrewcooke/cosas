
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


CtrlDamper::CtrlDamper(uint8_t lo, uint8_t hi) : thresh_lo(lo), thresh_hi(hi) {};

bool CtrlDamper::append(uint8_t ctrl, uint16_t now, uint16_t prev) {
  if (ctrl < N_KNOBS) {
    latest[Prev][ctrl] = average[Prev][ctrl].next(prev);
    uint16_t thresh = ctrl == active ? thresh_lo : thresh_hi;
    latest[Now][ctrl] = average[Now][ctrl].next_or(now, thresh, SKIP);
    if (latest[Now][ctrl] != SKIP) {
      active = ctrl;
      return true;
    } else {
      return false;
    }
  } else {
    // switch!
    active = ctrl;
    latest[Prev][ctrl] = prev;
    latest[Now][ctrl] = now;
    return true;
  }
}

uint16_t CtrlDamper::get(uint8_t knob, When when) {
  return latest[when][knob];
}


