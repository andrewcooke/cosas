
#include <numeric>
#include <iostream>
#include <cstdint>

#include "maths.h"
#include "modulators.h"


Merge::Merge(const Node& w, const Node& d, Balance bal)
  : wet(w), dry(d), balance(bal) {};

int16_t Merge::next(int32_t tick, int32_t phi) const {
  return balance.combine(wet.next(tick, phi), dry.next(tick, phi));
}


BaseMerge::BaseMerge(const Node& n, Weight w)
  : nodes(std::move(std::make_unique<std::vector<const Node*>>())), float_weights(std::move(std::make_unique<std::vector<float>>())), int16_weights(std::move(std::make_unique<std::vector<int16_t>>())) {
  add_node_wout_recalc(n, w);
  // cannot call virtual method in constructor so set reasonable default
  int16_weights->push_back(scale2mult_shift14(w.weight));
};

void BaseMerge::add_node_wout_recalc(const Node& n, Weight w) {
  nodes->push_back(&n);
  w.merge = this;
  float_weights->push_back(w.weight);
}

void BaseMerge::add_node(const Node& n, Weight w) {
  add_node_wout_recalc(n, w);
  recalculate_weights();
}

int16_t BaseMerge::next(int32_t tick, int32_t phi) const {
  int32_t acc = 0;
  for (size_t i = 0; i < float_weights->size(); i++) {
    acc += mult_shift14(int16_weights->at(i), nodes->at(i)->next(tick, phi));
  }
  return clip_16(acc);
}

BaseMerge::Weight::Weight(float w) : weight(w) {};


MultiMerge::MultiMerge(const Node& n, Weight w) : BaseMerge(n, w) {}

void MultiMerge::recalculate_weights() {
  // distribute equally
  int16_weights->clear();
  float total_weight = accumulate(float_weights->begin() + 1, float_weights->end(), 0.0);
  for (size_t i = 1; i < float_weights->size(); i++) {
    if (total_weight == 0) {
      int16_weights->push_back(0);
    } else {
      int16_weights->push_back(scale2mult_shift14(float_weights->at(i) / total_weight));
    }
  }
}


PriorityMerge::PriorityMerge(const Node& n, Weight w) : BaseMerge(n, w) {}

void PriorityMerge::recalculate_weights() {
  // first node gets the full amount described by it's weight
  int16_weights->clear();
  int16_t main_weight = scale2mult_shift14(float_weights->at(0));
  float available = 1 - float_weights->at(0);
  int16_weights->push_back(main_weight);
  // other nodes are divided in proportion to their relative weights
  float total_remaining = accumulate(float_weights->begin() + 1, float_weights->end(), 0.0);
  for (size_t i = 1; i < float_weights->size(); i++) {
    if (total_remaining == 0) {
      int16_weights->push_back(0);
    } else {
      int16_weights->push_back(scale2mult_shift14(float_weights->at(i) * available / total_remaining));
    }
  }
}

TEST_CASE("PriorityMerge") {
  Constant c1 = Constant(100); BaseMerge::Weight w1 = BaseMerge::Weight(0.5);
  Constant c2 = Constant(30); BaseMerge::Weight w2 = BaseMerge::Weight(0.1);
  Constant c3 = Constant(90); BaseMerge::Weight w3 = BaseMerge::Weight(0.2);
  PriorityMerge m = PriorityMerge(c1, w1);
  m.add_node(c2, w2);
  m.add_node(c3, w3);
  CHECK(m.next(0, 0) == 50 + 5 + 30 - 2);  // close enough?
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

