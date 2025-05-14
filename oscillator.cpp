
import std;
using namespace std;

#include "constants.h"
#include "oscillator.h"
#include "engine.h"


Oscillator::Oscillator(Wavetable& wave, unique_ptr<Frequency> freq)
  : Oscillator(wave, move(freq), move(make_unique<Amplitude>(1))) {};

Oscillator::Oscillator(Wavetable& wave, unique_ptr<Frequency> freq, unique_ptr<Amplitude> amp)
  : wavetable(wave), frequency(move(freq)), amplitude(move(amp)) {};

uint16_t Oscillator::next(int64_t tick, int32_t phi) {
  return amplitude->scale(wavetable.next(tick * frequency->get(), phi));
}

void Oscillator::set_freq_abs(uint16_t freq) {
  frequency = move(make_unique<AbsoluteFreq>(freq));
}

void Oscillator::set_freq_ratio(const Manager& manager, float ratio) {
  frequency = move(make_unique<RelativeFreq>(manager.get_root(), ratio));
}

const Frequency& Oscillator::get_frequency() {
  return *frequency;
}
