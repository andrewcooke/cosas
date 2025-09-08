
#include <numeric>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <memory>

#include "cosas/constants.h"
#include "cosas/maths.h"
#include "cosas/transformers.h"

#include "cosas/debug.h"

SingleFloat::SingleFloat(RelSource& nd, float v, float scale, float linearity, bool log, float lo, float hi)
  : SingleSource(nd), value(v), param(Value(this, scale, linearity, log, lo, hi)) {};

SingleFloat::Value::Value(SingleFloat* p, float scale, float linearity, bool log, float lo, float hi)
  : Param(scale, linearity, log, lo, hi), parent(p) {};

void SingleFloat::Value::set(float v) {
  parent->value = v;
}

float SingleFloat::Value::get() {
  return parent->value;
}


GainFloat::GainFloat(RelSource& nd, float amp, float hi)
  : SingleFloat(nd, amp, 1, 1, true, 0, hi) {};

int16_t GainFloat::next(const int32_t delta, const int32_t phi)  {
  const int16_t a = src.next(delta, phi);
  return clip_16(value * static_cast<float>(a));
}

SingleFloat::Value& GainFloat::get_amp() {
  return param;
}


Single14::Single14(RelSource& nd, const float v, float scale, float linearity, bool log, float lo, float hi)
  : SingleSource(nd), value(scale2mult_shift14(v)), param(Value(this, scale, linearity, log, lo, hi)), v(v) {};

Single14::Value::Value(Single14* p, float scale, float linearity, bool log, float lo, float hi)
  : Param(scale, linearity, log, lo, hi), parent(p) {};

void Single14::Value::set(const float v) {
  parent->value = scale2mult_shift14(v);
}

float Single14::Value::get() {
  return unscale2mult_shift14(parent->value);
}


Gain14::Gain14(RelSource& nd, const float amp, bool log)
  : Single14(nd, amp, 1, 1, log, log ? -3 : 0, log ? 3 : 2) {};

int16_t Gain14::next(const int32_t delta, const int32_t phi) {
  int16_t a = src.next(delta, phi);
  int16_t b = mult_shift14(value, a);
  return b;
}

Single14::Value& Gain14::get_amp() {
  return param;
}


Gain::Gain(RelSource& nd, float amp, bool log) : Gain14(nd, amp, log) {};


// these (float based) may be too slow?


FloatFunc::FloatFunc(RelSource& nd, float v, float scale, float linearity, bool log, float lo, float hi)
  : SingleFloat(nd, v, scale, linearity, log, lo, hi) {};

int16_t FloatFunc::next(const int32_t delta, const int32_t phi) {
  const int16_t sample = src.next(delta, phi);
  const bool neg = sample < 0;
  const float x = static_cast<float>(abs(sample)) / static_cast<float>(SAMPLE_MAX);
  const float y = func(x);
  int16_t new_sample = clip_16(SAMPLE_MAX * y);
  if (neg) new_sample = -new_sample;
  return new_sample;
}


Compander::Compander(RelSource& nd, float gamma) : FloatFunc(nd, gamma, 0.5, 1, false, -10, 10) {};

auto Compander::func(float x) const -> float {
  return powf(x, value);
}


Folder::Folder(RelSource& nd, float k) : FloatFunc(nd, k, 0.5, 1, false, 0, 2) {};

// first half goes from flat to curve
// second half actually folds
float Folder::func(const float x) const {
  if (value < 1) return x * (1 + value * (1 - x));
  else return 1.0f - powf(value * x - 1, 2);
}

SingleFloat::Value& Folder::get_fold() {
  return param;
}



Boxcar::Boxcar(RelSource& nd, size_t l)
  : SingleSource(nd), cbuf(std::move(std::make_unique<CircBuffer>(l))), param(Length(this)) {}

Boxcar::CircBuffer::CircBuffer(size_t l)
  : sums(std::move(std::make_unique<std::vector<int32_t>>(l, 0))), circular_idx(0) {}

auto Boxcar::CircBuffer::next(const int16_t cur) const -> int16_t {
  for (int32_t& s : *sums) s += cur;
  const int32_t next = (*sums)[circular_idx];
  (*sums)[circular_idx] = 0;
  circular_idx = (circular_idx + 1) % sums->size();
  return clip_16(next / static_cast<int32_t>(sums->size()));
};

size_t Boxcar::CircBuffer::size() {
  return sums->size();
}


Boxcar::Length::Length(Boxcar* p)
  : Param(1, 1, false, 0, MAX_BOXCAR), parent(p) {}

void Boxcar::Length::set(const float v) {
  size_t l;
  l = static_cast<size_t>(std::min(static_cast<float>(MAX_BOXCAR), std::max(1.0f, v)));
  parent->cbuf = std::move(std::make_unique<CircBuffer>(l));
}

float Boxcar::Length::get() {
  return parent->cbuf->size();
}


int16_t Boxcar::next(int32_t delta, int32_t phi) {
  return cbuf->next(src.next(delta, phi));
}

Boxcar::Length& Boxcar::get_len() {
  return param;
}


MergeFloat::MergeFloat(RelSource& n, const float w)
  : weights(std::move(std::make_unique<std::vector<Weight>>())),
    sources(std::move(std::make_unique<std::vector<RelSource*>>())),
    given_weights(std::move(std::make_unique<std::vector<float>>())),
    norm_weights(std::move(std::make_unique<std::vector<float>>())) {
  add_source(n, w);
}

void MergeFloat::add_source(RelSource& n, const float w) {
  sources->push_back(&n);
  given_weights->push_back(w);
  weights->push_back(Weight(this, weights->size()));
  normalize();
}

void MergeFloat::normalize() {
  std::unique_ptr<std::vector<float>> new_norm_weights = std::make_unique<std::vector<float>>();
  float weight_zero = given_weights->at(0);
  float other_weight = static_cast<float>(accumulate(given_weights->begin() + 1, given_weights->end(), 0.0));
  new_norm_weights->push_back(weight_zero);
  for (size_t i = 1; i < given_weights->size(); i++) {
    new_norm_weights->push_back((1 - weight_zero) * given_weights->at(i) / other_weight);
  }
  norm_weights = std::move(new_norm_weights);
}

MergeFloat::Weight& MergeFloat::get_weight(size_t i) const {
  return weights->at(i);
}

int16_t MergeFloat::next(const int32_t tick, const int32_t phi) {
  float acc = 0;
  for (size_t i = 0; i < norm_weights->size(); i++) {
    acc += norm_weights->at(i) * static_cast<float>(sources->at(i)->next(tick, phi));
  }
  return clip_16(acc + 0.5f);  // round to nearest
}

MergeFloat::Weight::Weight(MergeFloat* m, size_t i)
  : Param(0.5, 1, false, 0, 1), merge(m), idx(i) {}

void MergeFloat::Weight::set(float v) {
  merge->given_weights->at(idx) = v;
  merge->normalize();
}

float MergeFloat::Weight::get() {
  return merge->given_weights->at(idx);
}


Merge14::Merge14(RelSource& n, const float w)
  : MergeFloat(n, w), uint16_weights(std::move(std::make_unique<std::vector<uint16_t>>())) {}

void Merge14::normalize() {
  MergeFloat::normalize();
  auto new_uint16_weights = std::make_unique<std::vector<uint16_t>>();
  for (float w: *norm_weights) {
    new_uint16_weights->push_back(scale2mult_shift14(w));
  }
  uint16_weights = std::move(new_uint16_weights);
}

int16_t Merge14::next(const int32_t tick, const int32_t phi) {
  int32_t acc = 0;
  for (size_t i = 0; i < uint16_weights->size(); i++) {
    acc += mult_shift14(uint16_weights->at(i), sources->at(i)->next(tick, phi));
  }
  return clip_16(acc);
}


Merge::Merge(RelSource& n, const float w) : Merge14(n, w) {};

