
#include <cmath>

#include "weas/knobs.h"


KnobChange::~KnobChange() {
  knob->apply_change();
}


KnobChange Knob::handle_knob_change(uint16_t now, uint16_t prev) {
  normalized = std::max(0.0f, std::min(1.0f, normalized + static_cast<float>(now - prev) / 4095));
  // KnobChange::Highlight h = (abs(now - 2048) < 16 || now < 8 || now > 4087) ? KnobChange::Highlight::Yes : KnobChange::Highlight::No;
  return KnobChange(this, normalized, KnobChange::Highlight::No);
}

void Knob::apply_change() {
  // TODO - something would happen here (more expensive than just changing LEDs)
}
