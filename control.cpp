
#include <iostream>
#include <algorithm>
#include <cmath>

#include "doctest.h"

#include "control.h"


Delegate::Delegate(Input& del) : delegate(del) {};


Change::Change(Input& del) : Delegate(del) {};

void Change::set(float v) {
  if (v != prev) {
    delegate.set(v);
    prev = v;
  }
}


Blank::Blank() {};

void Blank::set(float /* v */) {};


Sigmoid::Sigmoid(Input& del, float lin)
  : Delegate(del), linear(lin) {};

void Sigmoid::set(float v) {
  float v2 = v - 0.5;
  // https://www.desmos.com/calculator/rbmvnpsmrm
  delegate.set(4 * (1 - linear) * pow(v2, 3) + linear * v2 + 0.5);
}


class Save : public Input {

public:

  Save(float *sav) : saved(sav) {};
  void set(float val) override {*saved = val;}
  
private:
  
  float *saved;
  
};
  

TEST_CASE("Sigmoid") {

  float x;
  std::unique_ptr<Save> save = std::make_unique<Save>(&x);

  Sigmoid(*save, 1).set(0); CHECK(x == 0);
  Sigmoid(*save, 1).set(1); CHECK(x == 1);
  Sigmoid(*save, 1).set(0.5); CHECK(x == 0.5);
  Sigmoid(*save, 1).set(0.25); CHECK(x == 0.25);
  Sigmoid(*save, 1).set(0.75); CHECK(x == 0.75);

  // these are much closer to 0.5
  Sigmoid(*save, 0).set(0); CHECK(x == 0);
  Sigmoid(*save, 0).set(1); CHECK(x == 1);
  Sigmoid(*save, 0).set(0.5); CHECK(x == 0.5);
  Sigmoid(*save, 0).set(0.25); CHECK(x == 0.4375);
  Sigmoid(*save, 0).set(0.75); CHECK(x == 0.5625);
  
}


Exp::Exp(Input& del) : Delegate(del) {};

void Exp::set(float v) {
  delegate.set(pow(10, 2 * (v - 0.5)));
};


TEST_CASE("Exp") {

  float x;
  std::unique_ptr<Save> save = std::make_unique<Save>(&x);
  
  Exp(*save).set(0); CHECK(x == doctest::Approx(0.1));
  Exp(*save).set(0.5); CHECK(x == doctest::Approx(1));
  Exp(*save).set(1); CHECK(x == doctest::Approx(10));

};


Range::Range(Input& del, float c, float lo, float hi)
  : Delegate(del), current(c), low(lo), high(hi) {};

void Range::set(float value) {
  // handle disconnect here
  value = std::max(low, std::min(high, update(value)));
  std::cerr << "range " << value << std::endl;
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
