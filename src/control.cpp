
#include <iostream>
#include <algorithm>
#include <cmath>

#include "cosas/control.h"


Delegate::Delegate(Input& del) : delegate(del) {}


Change::Change(Input& del) : Delegate(del) {}

void Change::set(float v) {
  if (v != prev) {
    delegate.set(v);
    prev = v;
  }
}


Blank::Blank() : delegate(nullptr) {}

void Blank::set(float v) {
  if (delegate != nullptr) delegate->set(v);
}

void Blank::unblank(Input* del) {
  delegate = del;
}


Sigmoid::Sigmoid(Input& del, const float lin)
  : Delegate(del), linear(lin) {};

void Sigmoid::set(float v) {
  const float v2 = v - 0.5f;
  // https://www.desmos.com/calculator/rbmvnpsmrm
  const float v3 = 4.0f * (1.0f - linear) * powf(v2, 3.0f) + linear * v2 + 0.5f;
  delegate.set(v3);
}


Exp::Exp(Input& del) : Delegate(del) {};

void Exp::set(float v) {
  delegate.set(powf(10.0f, 2.0f * (v - 0.5f)));
}


Range::Range(Input& del, float c, float lo, float hi)
  : Delegate(del), current(c), low(lo), high(hi) {};

void Range::set(float value) {
  // handle disconnect here (which will set current)
  value = std::max(low, std::min(high, update(value)));
  delegate.set(value);
}


Additive::Additive(Input& del, float c, float lo, float hi)
  : Range(del, c, lo, hi) {};

float Additive::update(float val) {
  return current + val;
}


Multiplicative::Multiplicative(Input& del, float c, float lo, float hi)
  : Range(del, c, lo, hi) {};

float Multiplicative::update(float val) {
  return current * val;
}
