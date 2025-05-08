
#include <cstdint>

#include "constants.h"
#include "model.h"


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
  return clip(((s1 - sample_zero) * (s2 - sample_zero)) / sample_zero + sample_zero);
};

MixedAM::MixedAM(const Source& src1, const Source& src2, const Amplitude& amp, const Balance& bal)
  : am(AM(src1, src2)), mixer(Mixer(src1, am, amp, bal)) {};

uint16_t MixedAM::next(int64_t tick, int32_t phi) const {
  return mixer.next(tick, phi);
}
