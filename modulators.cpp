
#include <cstdint>

#include "constants.h"
#include "maths.h"
#include "modulators.h"


Gain::Gain(Node& nd, const Amplitude& amp)
  : node(nd), amplitude(amp) {};

uint16_t Gain::next(int64_t tick, int32_t phi) {
  return amplitude.scale(node.next(tick, phi));
}


Merge::Merge(Node& nd1, Node& nd2, const Balance& bal)
  : node1(nd1), node2(nd2), balance(bal) {};

uint16_t Merge::next(int64_t tick, int32_t phi) {
  return balance.combine(node1.next(tick, phi), node2.next(tick, phi));
}


Mixer::Mixer(Node& nd1, Node& nd2, const Amplitude& amp, const Balance& bal)
  : node1(nd1), node2(nd2), amplitude(amp), balance(bal) {};

uint16_t Mixer::next(int64_t tick, int32_t phi) {
  return amplitude.scale(balance.combine(node1.next(tick, phi), node2.next(tick, phi)));
}


FM::FM(Node& car, Node& mod)
  : carrier(car), modulator(mod) {};

uint16_t FM::next(int64_t tick, int32_t phi) {
  int32_t mod = modulator.next(tick, phi);
  return carrier.next(tick, phi + mod - sample_zero);
};


MixedFM::MixedFM(Node& car, Node& mod, const Amplitude& amp, const Balance& bal)
  : fm(FM(car, mod)), mixer(Mixer(car, fm, amp, bal)) {};

uint16_t MixedFM::next(int64_t tick, int32_t phi) {
  return mixer.next(tick, phi);
}


AM::AM(Node& nd1, Node& nd2)
  : node1(nd1), node2(nd2) {};

uint16_t AM::next(int64_t tick, int32_t phi) {
  int32_t s1 = node1.next(tick, phi);
  int32_t s2 = node2.next(tick, phi);
  return clip_u16(((s1 - sample_zero) * (s2 - sample_zero)) / sample_zero + sample_zero);
};

MixedAM::MixedAM(Node& nd1, Node& nd2, const Amplitude& amp, const Balance& bal)
  : am(AM(nd1, nd2)), mixer(Mixer(nd1, am, amp, bal)) {};

uint16_t MixedAM::next(int64_t tick, int32_t phi) {
  return mixer.next(tick, phi);
}


