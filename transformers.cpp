
#include <numeric>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <memory>

#include "doctest.h"
#include "constants.h"
#include "maths.h"
#include "transformers.h"


SingleFloat::SingleFloat(const Node& nd, float v)
  : SingleNode(nd), value(v), param(Value(this)) {};

SingleFloat::Value& SingleFloat::get_param() {
  return param;
}

SingleFloat::Value::Value(SingleFloat* p) : parent(p) {};

void SingleFloat::Value::set(float v) {
  parent->value = v;
}


GainFloat::GainFloat(const Node& nd, float amp) : SingleFloat(nd, amp) {};

int16_t GainFloat::next(int32_t tick, int32_t phi) const {
  int16_t a = node.next(tick, phi);
  int16_t b = clip_16(value * a);
  return b;  
}

TEST_CASE("GainFloat") {
  Constant c = Constant(100);
  GainFloat g = GainFloat(c, 1);
  CHECK(g.next(123, 0) == 100);
  g.get_param().set(0.1);
  CHECK(g.next(123, 0) == 10);
}


Single14::Single14(const Node& nd, float v)
  : SingleNode(nd), value(scale2mult_shift14(v)), param(Value(this)) {};

Single14::Value& Single14::get_param() {
  return param;
}

Single14::Value::Value(Single14* p) : parent(p) {};

void Single14::Value::set(float v) {
  parent->value = scale2mult_shift14(v);
}


Gain14::Gain14(const Node& nd, float amp) : Single14(nd, amp) {};

int16_t Gain14::next(int32_t tick, int32_t phi) const {
  int16_t a = node.next(tick, phi);
  int16_t b = mult_shift14(value, a);
  return b;  
}

TEST_CASE("Gain14") {
  Constant c = Constant(100);
  Gain14 g = Gain14(c, 1);
  CHECK(g.next(123, 0) == 100);
  g.get_param().set(0.1);
  CHECK(g.next(123, 0) == 9);  // almost
}


Gain::Gain(const Node& nd, float amp) : Gain14(nd, amp) {};


// these (float based) may be too slow?


FloatFunc::FloatFunc(const Node& nd, float v) : SingleFloat(nd, v) {};

int16_t FloatFunc::next(int32_t tick, int32_t phi) const {
  int16_t sample = node.next(tick, phi);
  bool neg = sample < 0;
  float x = abs(sample) / static_cast<float>(sample_max);
  float y = func(x);
  int16_t new_sample = clip_16(sample_max * y);
  if (neg) new_sample = -new_sample;
  return new_sample;
}


Compander::Compander(const Node& nd, float gamma) : FloatFunc(nd, gamma) {};

float Compander::func(float x) const {
  return pow(x, value);
}


Folder::Folder(const Node& nd, float k) : FloatFunc(nd, k) {};

// first half goes from flat to curve
// second half actually folds
float Folder::func(float x) const {
  if (value < 1) return x * (1 + value * (1 - x));
  else return 1 - pow(value * x - 1, 2);
}

TEST_CASE("Folder") {

  Constant c1234 = Constant(1234);  // random +ve value
  Folder f0_12 = Folder(c1234, 0);
  CHECK(f0_12.next(0, 0) == 1234);
  f0_12.get_param().set(1);
  CHECK(f0_12.next(0, 0) == 2421);  // not sure if correct, but more +ve
  f0_12.get_param().set(2);
  CHECK(f0_12.next(0, 0) == 4750);  // not sure if correct
  Constant cm1234 = Constant(-1234);
  Folder f1_12x = Folder(cm1234, 1);
  CHECK(f1_12x.next(0, 0) == -2421);  // symmetrical
  
  Constant cmax = Constant(sample_max);  
  Folder f0_max = Folder(cmax, 0);
  CHECK(f0_max.next(0, 0) == sample_max);
  f0_max.get_param().set(1);
  CHECK(f0_max.next(0, 0) == sample_max);
  f0_max.get_param().set(2);
  CHECK(f0_max.next(0, 0) == 0);

}


MeanFilter::MeanFilter(const Node& nd, Length l)
  : SingleNode(nd), len(l), cbuf(std::move(std::make_unique<CircBuffer>(l.len))) {
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
  : nodes(std::move(std::make_unique<std::vector<const Node*>>())), float_weights(std::move(std::make_unique<std::vector<float>>())), uint16_weights(std::move(std::make_unique<std::vector<uint16_t>>())) {
  add_node_wout_recalc(n, w);
  // cannot call virtual method in constructor so set reasonable default
  uint16_weights->push_back(scale2mult_shift14(w.weight));
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
    acc += mult_shift14(uint16_weights->at(i), nodes->at(i)->next(tick, phi));
  }
  return clip_16(acc);
}

BaseMerge::Weight::Weight(float w) : weight(w) {};


MultiMerge::MultiMerge(const Node& n, Weight w) : BaseMerge(n, w) {}

void MultiMerge::recalculate_weights() {
  // distribute equally
  std::unique_ptr<std::vector<uint16_t>> new_weights = std::make_unique<std::vector<uint16_t>>();
  float total_weight = accumulate(float_weights->begin() + 1, float_weights->end(), 0.0);
  for (size_t i = 1; i < float_weights->size(); i++) {
    if (total_weight == 0) {
      new_weights->push_back(0);
    } else {
      new_weights->push_back(scale2mult_shift14(float_weights->at(i) / total_weight));
    }
  }
  uint16_weights = move(new_weights);  // atomic?
}


PriorityMerge::PriorityMerge(const Node& n, Weight w) : BaseMerge(n, w) {}

void PriorityMerge::recalculate_weights() {
  // first node gets the full amount described by it's weight
  std::unique_ptr<std::vector<uint16_t>> new_weights = std::make_unique<std::vector<uint16_t>>();
  uint16_t main_weight = scale2mult_shift14(float_weights->at(0));
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
  uint16_weights = move(new_weights);  // atomic?
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


