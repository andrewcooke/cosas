
#include "constants.h"
#include "oscillator.h"
#include "engine.h"


Oscillator::Oscillator(Wavedex w, std::unique_ptr<Frequency> f)
  : wavedex(w), frequency(std::move(f)) {};

int16_t Oscillator::next(int32_t tick, int32_t phi) const {
  uint32_t freq = frequency->get_frequency();
  if (phi != 0) {
    // normalise - treat phi as fraction of max * freq
    phi = (phi * freq) >> (sample_depth - 1);
  }
  return wavedex.get_wavetable().next(tick * freq, phi);
}

