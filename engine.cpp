
#include <cstdint>

#include "constants.h"
#include "maths.h"
#include "engine.h"
#include "oscillator.h"


Mixer::Mixer(const Source& src1, const Source& src2, const Amplitude& amp, const Balance& bal)
  : source1(src1), source2(src2), amplitude(amp), balance(bal) {};

uint16_t Mixer::next(int64_t tick, int32_t phi) const {
  return amplitude.scale(balance.combine(source1.next(tick, phi), source2.next(tick, phi)));
}


FM::FM(const Source& car, const Source& mod)
  : carrier(car), modulator(mod) {};

uint16_t FM::next(int64_t tick, int32_t phi) const {
  int32_t mod = modulator.next(tick, phi);
  return carrier.next(tick, phi + mod - sample_zero);
};


MixedFM::MixedFM(const Source& car, const Source& mod, const Amplitude& amp, const Balance& bal)
  : fm(FM(car, mod)), mixer(Mixer(car, fm, amp, bal)) {};

uint16_t MixedFM::next(int64_t tick, int32_t phi) const {
  return mixer.next(tick, phi);
}


AM::AM(const Source& src1, const Source& src2)
  : source1(src1), source2(src2) {};

uint16_t AM::next(int64_t tick, int32_t phi) const {
  int32_t s1 = source1.next(tick, phi);
  int32_t s2 = source2.next(tick, phi);
  return clip_u16(((s1 - sample_zero) * (s2 - sample_zero)) / sample_zero + sample_zero);
};

MixedAM::MixedAM(const Source& src1, const Source& src2, const Amplitude& amp, const Balance& bal)
  : am(AM(src1, src2)), mixer(Mixer(src1, am, amp, bal)) {};

uint16_t MixedAM::next(int64_t tick, int32_t phi) const {
  return mixer.next(tick, phi);
}


Manager::Manager() {
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
				 (*wavetables.at(0), move(amplitude),
				  move(f))));
    } else {
      oscillators.push_back(move(make_unique<Oscillator>
				 (*wavetables.at(0), move(amplitude),
				  move(make_unique<RelativeFreq>(oscillators.at(0)->get_frequency(), 1, 1)))));
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
