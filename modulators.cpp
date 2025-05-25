
#include <iostream>
#include <cstdint>

#include "maths.h"
#include "modulators.h"


Merge::Merge(const Node& w, const Node& d, Balance bal)
  : wet(w), dry(d), balance(bal) {};

int16_t Merge::next(int32_t tick, int32_t phi) const {
  return balance.combine(wet.next(tick, phi), dry.next(tick, phi));
}


Mixer::Mixer(const Node& nd1, const Node& nd2, Amplitude amp, Balance bal)
  : node1(nd1), node2(nd2), amplitude(amp), balance(bal) {};

int16_t Mixer::next(int32_t tick, int32_t phi) const {
  return amplitude.scale(balance.combine(node1.next(tick, phi), node2.next(tick, phi)));
}


FM::FM(const Node& car, const Node& mod)
  : carrier(car), modulator(mod) {};

int16_t FM::next(int32_t tick, int32_t phi) const {
  int32_t delta = modulator.next(tick, phi);
  return carrier.next(tick, phi + delta);
};


MixedFM::MixedFM(const Node& car, const Node& mod, Amplitude amp, Balance bal)
  : fm(FM(car, mod)), mixer(Mixer(car, fm, amp, bal)) {};

int16_t MixedFM::next(int32_t tick, int32_t phi) const {
  return mixer.next(tick, phi);
}


ModularFM::ModularFM(const Node& car, const Node& mod, Amplitude amp, Balance bal)
  : gain(Gain(mod, amp)), fm(FM(car, gain)), merge(Merge(fm, car, bal)) {};

int16_t ModularFM::next(int32_t tick, int32_t phi) const {
  return merge.next(tick, phi);
}


AM::AM(const Node& nd1, const Node& nd2)
  : node1(nd1), node2(nd2) {};

int16_t AM::next(int32_t tick, int32_t phi) const {
  int32_t s1 = node1.next(tick, phi);
  int32_t s2 = node2.next(tick, phi);
  return clip_16((s1 * s2) >> 16);
};

MixedAM::MixedAM(const Node& nd1, const Node& nd2, Amplitude amp, Balance bal)
  : am(AM(nd1, nd2)), mixer(Mixer(nd1, am, amp, bal)) {};

int16_t MixedAM::next(int32_t tick, int32_t phi) const {
  return mixer.next(tick, phi);
}

