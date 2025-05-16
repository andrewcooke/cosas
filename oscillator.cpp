
#include "oscillator.h"
#include "engine.h"


Oscillator::Oscillator(Wavetable& wave, std::unique_ptr<Frequency> freq)
  : Oscillator(wave, std::move(freq), Amplitude(1)) {};

Oscillator::Oscillator(Wavetable& wave, std::unique_ptr<Frequency> freq, Amplitude amp)
  : wavetable(wave), frequency(std::move(freq)), amplitude(amp) {};

int16_t Oscillator::next(int32_t tick, int32_t phi) {
  return amplitude.scale(wavetable.next(tick * frequency->get_frequency(), phi));
}

const Frequency& Oscillator::get_frequency() {
  return *frequency;
}
