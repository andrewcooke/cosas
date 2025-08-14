
#include <cmath>
#include <iostream>

#include "weas/knobs.h"


KnobChange::~KnobChange() {
  knob->apply_change();
}


KnobChange Knob::handle_knob_change(uint16_t now, uint16_t prev) {
  // normalized = clip(sigmoid(now, prev));
  // normalized = linear(now, prev);
  normalized = absolute(now, prev);
  return KnobChange(this, normalized, ends());
}

KnobChange::Highlight Knob::ends() {
  KnobChange::Highlight highlight = KnobChange::No;
  if (normalized < 0.1f || normalized > 0.9f) highlight = KnobChange::Near;
  if (normalized < 0.01f || normalized > 0.99f) highlight = KnobChange::Yes;
  return highlight;
}

void Knob::apply_change() {
  float val = lo + (hi - lo) * normalized;
  if (log) val = powf(10, val);
  // TODO - something with val
  std::cout << val << std::endl;
}

float Knob::clip(float n) {
  return std::max(0.0f, std::min(1.0f, n));
}

float Knob::absolute(uint16_t now, uint16_t /* prev */) {
  return static_cast<float>(now) / 4095;
}

float Knob::linear(uint16_t now, uint16_t prev) {
  return clip(normalized + scale * static_cast<float>(now - prev) / 4095);
}

float Knob::sigmoid(uint16_t now, uint16_t prev) {
  float x1 = static_cast<float>(now - 2048) / 4095;
  float x0 = static_cast<float>(prev - 2048) / 4095;
  float y1 =  4.0f * (1.0f - linearity) * powf(x1, 3.0f) + linearity * x1 + 0.5f;
  float y0 =  4.0f * (1.0f - linearity) * powf(x0, 3.0f) + linearity * x0 + 0.5f;
  return clip(normalized + scale * (y1 - y0));
}
