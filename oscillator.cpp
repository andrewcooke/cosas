
#include "constants.h"
#include "oscillator.h"
#include "engine.h"


Oscillator::Oscillator(Wavetable& wave, std::unique_ptr<Frequency> freq)
  : wavetable(wave), frequency(std::move(freq)) {};

int16_t Oscillator::next(int32_t tick, int32_t phi) const {
  uint32_t freq = frequency->get_frequency();
  if (phi != 0) {
    // normalise - treat phi as fraction of max * freq
    phi = (phi * freq) >> (sample_depth - 1);
  }
  return wavetable.next(tick * freq, phi);
}

const Frequency& Oscillator::get_frequency() {
  return *frequency;
}
