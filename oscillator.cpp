
#include <iostream>

#include "constants.h"
#include "oscillator.h"
#include "engine.h"


Oscillator::Oscillator(Wavedex w, std::unique_ptr<Frequency> f)
  : wavedex(w), frequency(std::move(f)) {};

int16_t Oscillator::next(int32_t tick, int32_t phi) const {
  int64_t freq = frequency->get_frequency();
  // convert phi from simple_min-sample_max to -pi-pi (kinda)
  int64_t phi_tmp = phi * freq;
  // fudge allows more variation (phi limited to sample_max)
  // but may need to worry about gain sensitivity
  int32_t phi_frac = phi_tmp >> (sample_bits - 1 - phi_fudge_bits);
  return wavedex.get_wavetable().next(tick * freq, phi_frac);
  // return wavedex.get_wavetable().next(tick * freq, phi);
}

