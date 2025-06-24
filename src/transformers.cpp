
#include <numeric>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <memory>

#include "cosas/doctest.h"
#include "cosas/constants.h"
#include "cosas/maths.h"
#include "cosas/transformers.h"


SingleFloat::SingleFloat(const Node& nd, float v)
  : SingleNode(nd), value(v), param(Value(this)) {};

//SingleFloat::Value& SingleFloat::get_param() {
//  return param;
//}

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

SingleFloat::Value& GainFloat::get_amp() {
  return param;
}

TEST_CASE("GainFloat") {
  Constant c = Constant(100);
  GainFloat g = GainFloat(c, 1);
  CHECK(g.next(123, 0) == 100);
  g.get_amp().set(0.1);
  CHECK(g.next(123, 0) == 10);
}


Single14::Single14(const Node& nd, float v)
  : SingleNode(nd), value(scale2mult_shift14(v)), param(Value(this)) {};

//Single14::Value& Single14::get_param() {
//  return param;
//}

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

Single14::Value& Gain14::get_amp() {
  return param;
}

TEST_CASE("Gain14") {
  Constant c = Constant(100);
  Gain14 g = Gain14(c, 1);
  CHECK(g.next(123, 0) == 100);
  g.get_amp().set(0.1);
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

SingleFloat::Value& Folder::get_fold() {
  return param;
}



TEST_CASE("Folder") {

  Constant c1234 = Constant(1234);  // random +ve value
  Folder f0_12 = Folder(c1234, 0);
  CHECK(f0_12.next(0, 0) == 1234);
  f0_12.get_fold().set(1);
  CHECK(f0_12.next(0, 0) == 2421);  // not sure if correct, but more +ve
  f0_12.get_fold().set(2);
  CHECK(f0_12.next(0, 0) == 4750);  // not sure if correct
  Constant cm1234 = Constant(-1234);
  Folder f1_12x = Folder(cm1234, 1);
  CHECK(f1_12x.next(0, 0) == -2421);  // symmetrical
  
  Constant cmax = Constant(sample_max);  
  Folder f0_max = Folder(cmax, 0);
  CHECK(f0_max.next(0, 0) == sample_max);
  f0_max.get_fold().set(1);
  CHECK(f0_max.next(0, 0) == sample_max);
  f0_max.get_fold().set(2);
  CHECK(f0_max.next(0, 0) == 0);
}


Boxcar::Boxcar(const Node& nd, size_t l)
  : SingleNode(nd), cbuf(std::move(std::make_unique<CircBuffer>(l))), param(Length(this)) {}

Boxcar::CircBuffer::CircBuffer(size_t len)
  : sums(std::move(std::make_unique<std::vector<int32_t>>(len, 0))), circular_idx(0) {}

int16_t Boxcar::CircBuffer::next(int16_t cur) {
  for (int32_t& s : *sums) s += cur;
  int32_t next = (*sums)[circular_idx];
  (*sums)[circular_idx] = 0;
  circular_idx = (circular_idx + 1) % sums->size();
  return clip_16(next / static_cast<int32_t>(sums->size()));
};

Boxcar::Length::Length(Boxcar* p) : parent(p) {};

void Boxcar::Length::set(float v) {
  size_t l = static_cast<size_t>(std::min(static_cast<float>(MAX_BOXCAR), std::max(1.f, v)));
  parent->cbuf = std::move(std::make_unique<CircBuffer>(l));
}

int16_t Boxcar::next(int32_t tick, int32_t phi) const {
  return cbuf->next(node.next(tick, phi));
}

Boxcar::Length& Boxcar::get_len() {
  return param;
}

TEST_CASE("Boxcar") {
  Sequence s1 = Sequence({0, 0, 100});
  Boxcar b1 = Boxcar(s1, 3);
  CHECK(b1.next(0, 0) == 0);
  CHECK(b1.next(0, 0) == 0);
  CHECK(b1.next(0, 0) == 33);
  CHECK(b1.next(0, 0) == 33);
  CHECK(b1.next(0, 0) == 33);
  CHECK(b1.next(0, 0) == 0);
  CHECK(b1.next(0, 0) == 0);
  Sequence s2 = Sequence({0, 0, 100});
  Boxcar b2 = Boxcar(s2, 3);
  b2.get_len().set(1);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 100);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 0);
}


MergeFloat::MergeFloat(const Node& n, float w)
  : weights(std::move(std::make_unique<std::vector<Weight>>())),
    nodes(std::move(std::make_unique<std::vector<const Node*>>())),
    given_weights(std::move(std::make_unique<std::vector<float>>())),
    norm_weights(std::move(std::make_unique<std::vector<float>>())) {
  add_node(n, w);
}

void MergeFloat::add_node(const Node& n, float w) {
  nodes->push_back(&n);
  given_weights->push_back(w);
  weights->push_back(Weight(this, weights->size()));
  normalize();
}

void MergeFloat::normalize() {
  std::unique_ptr<std::vector<float>> new_norm_weights = std::make_unique<std::vector<float>>();
  float weight_zero = given_weights->at(0);
  float other_weight = accumulate(given_weights->begin() + 1, given_weights->end(), 0.0);
  new_norm_weights->push_back(weight_zero);
  for (size_t i = 1; i < given_weights->size(); i++) {
    new_norm_weights->push_back((1 - weight_zero) * given_weights->at(i) / other_weight);
  }
  norm_weights = std::move(new_norm_weights);
}

MergeFloat::Weight& MergeFloat::get_weight(size_t i) {
  return weights->at(i);
}

int16_t MergeFloat::next(int32_t tick, int32_t phi) const {
  float acc = 0;
  for (size_t i = 0; i < norm_weights->size(); i++) {
    acc += norm_weights->at(i) * nodes->at(i)->next(tick, phi);
  }
  return clip_16(acc + 0.5f);  // round to nearest
}

MergeFloat::Weight::Weight(MergeFloat* m, size_t i) : merge(m), idx(i) {}

void MergeFloat::Weight::set(float v) {
  merge->given_weights->at(idx) = v;
  merge->normalize();
}

TEST_CASE("MergeFloat") {
  Constant c1 = Constant(100);
  Constant c2 = Constant(30);
  Constant c3 = Constant(90);
  MergeFloat m = MergeFloat(c1, 0.5);
  m.add_node(c2, 0.1);
  m.add_node(c3, 0.2);
  CHECK(m.next(0, 0) == 50 + 5 + 30);
  m.get_weight(0).set(0);
  CHECK(m.next(0, 0) == 10 + 60);
}


Merge14::Merge14(const Node& n, float w) : MergeFloat(n, w), uint16_weights(std::move(std::make_unique<std::vector<uint16_t>>())) {}

void Merge14::normalize() {
  MergeFloat::normalize();
  std::unique_ptr<std::vector<uint16_t>> new_uint16_weights = std::make_unique<std::vector<uint16_t>>();  
  for (float w: *norm_weights) {
    new_uint16_weights->push_back(scale2mult_shift14(w));
  }
  uint16_weights = std::move(new_uint16_weights);
}

int16_t Merge14::next(int32_t tick, int32_t phi) const {
  int32_t acc = 0;
  for (size_t i = 0; i < uint16_weights->size(); i++) {
    acc += mult_shift14(uint16_weights->at(i), nodes->at(i)->next(tick, phi));
  }
  return clip_16(acc);
}

TEST_CASE("Merge14") {
  Constant c1 = Constant(100);
  Constant c2 = Constant(30);
  Constant c3 = Constant(90);
  Merge14 m = Merge14(c1, 0.5);
  m.add_node(c2, 0.1);
  m.add_node(c3, 0.2);
  CHECK(m.next(0, 0) == 50 + 5 + 30 - 2);  // close enough?
  m.get_weight(0).set(0);
  CHECK(m.next(0, 0) == 10 + 60 - 2);  // ditto
}


Merge::Merge(const Node& n, float w) : Merge14(n, w) {};



