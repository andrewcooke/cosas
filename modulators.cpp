
#include <iostream>
#include <cstdint>

#include "maths.h"
#include "modulators.h"


Merge::Merge(Node& nd1, Node& nd2, Balance bal)
  : node1(nd1), node2(nd2), balance(bal) {};

int16_t Merge::next(int32_t tick, int32_t phi) {
  return balance.combine(node1.next(tick, phi), node2.next(tick, phi));
}


Mixer::Mixer(Node& nd1, Node& nd2, Amplitude amp, Balance bal)
  : node1(nd1), node2(nd2), amplitude(amp), balance(bal) {};

int16_t Mixer::next(int32_t tick, int32_t phi) {
  return amplitude.scale(balance.combine(node1.next(tick, phi), node2.next(tick, phi)));
}


FM::FM(Node& car, Node& mod)
  : carrier(car), modulator(mod) {};

int16_t FM::next(int32_t tick, int32_t phi) {
  int16_t mod = modulator.next(tick, phi);
  return carrier.next(tick, phi + mod);
};


MixedFM::MixedFM(Node& car, Node& mod, Amplitude amp, Balance bal)
  : fm(FM(car, mod)), mixer(Mixer(car, fm, amp, bal)) {};

int16_t MixedFM::next(int32_t tick, int32_t phi) {
  return mixer.next(tick, phi);
}


ModularFM::ModularFM(Node& car, Node& mod, Amplitude amp, Balance bal)
  : gain(Gain(mod, amp)), fm(FM(car, gain)), merge(Merge(car, fm, bal)) {};

int16_t ModularFM::next(int32_t tick, int32_t phi) {
  return merge.next(tick, phi);
}


AM::AM(Node& nd1, Node& nd2)
  : node1(nd1), node2(nd2) {};

int16_t AM::next(int32_t tick, int32_t phi) {
  int32_t s1 = node1.next(tick, phi);
  int32_t s2 = node2.next(tick, phi);
  return clip_16((s1 * s2) >> 16);
};

MixedAM::MixedAM(Node& nd1, Node& nd2, Amplitude amp, Balance bal)
  : am(AM(nd1, nd2)), mixer(Mixer(nd1, am, amp, bal)) {};

int16_t MixedAM::next(int32_t tick, int32_t phi) {
  return mixer.next(tick, phi);
}

