
#include "oscillator.h"
#include "engine.h"


Oscillator::Oscillator(Wavetable& wave, std::unique_ptr<Frequency> freq)
  : wavetable(wave), frequency(std::move(freq)) {};

int16_t Oscillator::next(int32_t tick, int32_t phi) {
  return wavetable.next(tick * frequency->get_frequency(), phi);
}

const Frequency& Oscillator::get_frequency() {
  return *frequency;
}
