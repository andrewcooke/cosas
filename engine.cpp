
#include <cstdint>

#include "constants.h"
#include "maths.h"
#include "engine.h"
#include "oscillator.h"


Manager::Manager(bool ext) : extended(ext) {
  init_wavetables();
  init_oscillators();
}

void Manager::init_wavetables() {
  wavetables.push_back(make_unique<Sine>(Sine()));
  wavetables.push_back(make_unique<Square>(Square()));
}

void Manager::init_oscillators() {
  
  for (size_t i = 0; i < max_oscillators; i++) {
    unique_ptr<Amplitude> amplitude = make_unique<Amplitude>(1);
    // the first frequency is absolute; the rest are relative
    if (i == 0) {
      unique_ptr<AbsoluteFreq> f = make_unique<AbsoluteFreq>(440);
      root = &*f;  // saved here for ease of access, but not the owner
      oscillators.push_back(move(make_unique<Oscillator>
				 (*wavetables.at(0), move(f), move(amplitude))));
    } else {
      oscillators.push_back(move(make_unique<Oscillator>
				 (*wavetables.at(0),
				  move(make_unique<RelativeFreq>(oscillators.at(0)->get_frequency(), 1, extended)),
				  move(amplitude))));
    }
  }
}

Oscillator& Manager::get_oscillator(size_t n) const {
  return *oscillators.at(n);
}

void Manager::set_root(uint16_t freq) {
  // unlike other frequencies / oscillators, the root (from which all
  // relative values are calculated)  must stay the "same" instance.
  // so it must be updated in-place so that relative changes work.
  root->set(freq);
}

const AbsoluteFreq& Manager::get_root() const {
  return *root;
}

bool Manager::is_extended() const {
  return extended;
}

