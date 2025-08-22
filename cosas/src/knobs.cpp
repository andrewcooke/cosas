
#include <cmath>
#include <iostream>

#include "cosas/knobs.h"

KnobChange::~KnobChange() {
  knob->apply_change();
}


KnobChange KnobHandler::handle_knob_change(uint16_t now, uint16_t prev) {
  normalized = sigmoid(now, prev);
  return KnobChange(this, normalized, ends());
}

KnobChange::Highlight KnobHandler::ends() {
  KnobChange::Highlight highlight = KnobChange::No;
  if (normalized < 0.1f || normalized > 0.9f) highlight = KnobChange::Near;
  if (normalized < 0.01f || normalized > 0.99f) highlight = KnobChange::Yes;
  return highlight;
}

void KnobHandler::apply_change() {
  float val = lo + (hi - lo) * normalized;
  if (log) val = powf(10, val);
  // TODO - something with val
}

float KnobHandler::clip(float n) {
  // aiming for [0, 1) here
  return std::max(0.0f, std::min(0.999999f, n));
}

float KnobHandler::sigmoid(uint16_t now, uint16_t prev) {
  float x1 = static_cast<float>(now - 2048) / 4095;
  float x0 = static_cast<float>(prev - 2048) / 4095;
  float y1 =  4.0f * (1.0f - linearity) * powf(x1, 3.0f) + linearity * x1 + 0.5f;
  float y0 =  4.0f * (1.0f - linearity) * powf(x0, 3.0f) + linearity * x0 + 0.5f;
  return clip(normalized + scale * (y1 - y0));
}
