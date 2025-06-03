
#include <numeric>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <memory>

#include "doctest.h"
#include "constants.h"
#include "maths.h"
#include "transformers.h"


Gain::Gain(const Node& nd, const Amplitude& amp)
  : NodeTransformer(nd), amplitude(amp) {
};

int16_t Gain::next(int32_t tick, int32_t phi) const {
  int16_t a = node.next(tick, phi);
  int16_t b = amplitude.scale(a);
  std::cerr << a << " gain " << b << std::endl;
  return b;
}

TEST_CASE("Gain") {
  Amplitude a = Amplitude();
  Constant c = Constant(220);
  Gain g = Gain(c, a);
  CHECK(g.next(0, 0) == 220);
  a.set(0.1);
  CHECK(g.next(0, 0) == 22);
}


// these (float based) may be too slow?

OneParFunc::OneParFunc(const Node& nd, float k)
  : NodeTransformer(nd), constant(k) {};

int16_t OneParFunc::next(int32_t tick, int32_t phi) const {
  int16_t sample = node.next(tick, phi);
  bool invert = sample < 0;
  float x = abs(sample) / static_cast<float>(sample_max);
  float y = func(constant, x);
  sample = clip_16(sample_max * y);
  if (invert) sample = -sample;
  return sample;
}


Compander::Compander(const Node& nd, float k)
  : OneParFunc(nd, k) {};

float Compander::func(float k, float x) const {
  return pow(x, k);
}


Folder::Folder(const Node& nd, float k)
  : OneParFunc(nd, k) {};

// first half goes from flat to curve
// second half actually folds
float Folder::func(float k, float x) const {
  if (k < 1) return x * (1 + k * (1 - x));
  else return 1 - pow(k * x - 1, 2);
}


TEST_CASE("Folder") {

  Constant c1234 = Constant(1234);  // random +ve value
  Folder f0_12 = Folder(c1234, 0);
  CHECK(f0_12.next(0, 0) == 1234);
  Folder f1_12 = Folder(c1234, 1);
  CHECK(f1_12.next(0, 0) == 2421);  // not sure if correct, but more +ve
  Constant cm1234 = Constant(-1234);
  Folder f1_12x = Folder(cm1234, 1);
  CHECK(f1_12x.next(0, 0) == -2421);  // symmetrical
  
  Constant cmax = Constant(sample_max);  
  Folder f0_max = Folder(cmax, 0);
  CHECK(f0_max.next(0, 0) == sample_max);
  Folder f1_max = Folder(cmax, 1);
  CHECK(f1_max.next(0, 0) == sample_max);
  Folder f2_max = Folder(cmax, 2);
  CHECK(f2_max.next(0, 0) == 0);

}


MeanFilter::MeanFilter(const Node& nd, Length l)
  : NodeTransformer(nd), len(l), cbuf(std::move(std::make_unique<CircBuffer>(l.len))) {
  len.filter = this;
};

MeanFilter::CircBuffer::CircBuffer(size_t len)
  : sums(std::move(std::make_unique<std::vector<int32_t>>(len, 0))), circular_idx(0) {};

int16_t MeanFilter::CircBuffer::next(int16_t cur) {
  for (int32_t& s : *sums) s += cur;
  int32_t next = (*sums)[circular_idx];
  (*sums)[circular_idx] = 0;
  circular_idx = (circular_idx + 1) % sums->size();
  return clip_16(next / static_cast<int32_t>(sums->size()));
};

MeanFilter::Length::Length(size_t l) : len(l) {};

int16_t MeanFilter::next(int32_t tick, int32_t phi) const {
  return cbuf->next(node.next(tick, phi));
}

TEST_CASE("MeanFilter") {

  Sequence s = Sequence({0, 0, 100});
  MeanFilter mf = MeanFilter(s, 3);
  CHECK(mf.next(0, 0) == 0);
  CHECK(mf.next(0, 0) == 0);
  CHECK(mf.next(0, 0) == 33);
  CHECK(mf.next(0, 0) == 33);
  CHECK(mf.next(0, 0) == 33);
  CHECK(mf.next(0, 0) == 0);
  CHECK(mf.next(0, 0) == 0);

}

BaseMerge::BaseMerge(const Node& n, Weight w)
  : nodes(std::move(std::make_unique<std::vector<const Node*>>())), float_weights(std::move(std::make_unique<std::vector<float>>())), int16_weights(std::move(std::make_unique<std::vector<int16_t>>())) {
  add_node_wout_recalc(n, w);
  // cannot call virtual method in constructor so set reasonable default
  int16_weights->push_back(scale2mult_shift14(w.weight));
}

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
  std::unique_ptr<std::vector<int16_t>> new_weights = std::make_unique<std::vector<int16_t>>();
  float total_weight = accumulate(float_weights->begin() + 1, float_weights->end(), 0.0);
  for (size_t i = 1; i < float_weights->size(); i++) {
    if (total_weight == 0) {
      new_weights->push_back(0);
    } else {
      new_weights->push_back(scale2mult_shift14(float_weights->at(i) / total_weight));
    }
  }
  int16_weights = move(new_weights);  // atomic?
}


PriorityMerge::PriorityMerge(const Node& n, Weight w) : BaseMerge(n, w) {}

void PriorityMerge::recalculate_weights() {
  // first node gets the full amount described by it's weight
  std::unique_ptr<std::vector<int16_t>> new_weights = std::make_unique<std::vector<int16_t>>();
  int16_t main_weight = scale2mult_shift14(float_weights->at(0));
  float available = 1 - float_weights->at(0);
  new_weights->push_back(main_weight);
  // other nodes are divided in proportion to their relative weights
  float total_remaining = accumulate(float_weights->begin() + 1, float_weights->end(), 0.0);
  for (size_t i = 1; i < float_weights->size(); i++) {
    if (total_remaining == 0) {
      new_weights->push_back(0);
    } else {
      new_weights->push_back(scale2mult_shift14(float_weights->at(i) * available / total_remaining));
    }
  }
  int16_weights = move(new_weights);  // atomic?
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


